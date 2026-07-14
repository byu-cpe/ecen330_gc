// https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/network/esp_now.html
// https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/freertos_additions.html#ring-buffers

#include <stdbool.h>
#include <stdlib.h> // malloc, free
#include <string.h> // memset, memcmp

#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_mac.h" // MACSTR, MAC2STR
#include "nvs_flash.h"
#include "esp_log.h"

#include "net.h"

// #define DEBUG 1
#define ENABLE_LONG_RANGE 1
#define DEFAULT_CHANNEL 1 // (0-14)
#define RINGBUF_SIZE 512
#define STACK_SZ 2048
#define PRIORITY 4 // send task priority
#define TPERIOD 200 // timer period in ms
#define TWAIT 100 // timer timeout period in ms
#define IS_BCAST(addr) (memcmp(addr, broadcast_mac, NET_ALEN) == 0)
#define IS_GROUP(addr) (memcmp(addr, group_mac, 1) == 0)

#define ERR_NONE 0
#define ERR_FAIL -1
#define ERR_RB_GET -2 // Ring buffer get (acquire, receive)
#define ERR_RB_RET -3 // Ring buffer return (complete)
#define ERR_SIZE -4
#define ERR_PEER -5

typedef enum {P_DATA, P_GROUP} type_e;

typedef struct {
	uint16_t type;
	uint8_t data[0];
} payload_t;

typedef struct {
	uint8_t addr[NET_ALEN];
	payload_t pay;
} packet_t;

static const char *TAG = "net";
static RingbufHandle_t rbuf_h, sbuf_h;
static TaskHandle_t task_h;
static TimerHandle_t timer_h;
static bool nvs_flash_f, esp_wifi_f, esp_now_f;
static volatile bool group_mode;
static volatile uint32_t group_id;
static const uint8_t broadcast_mac[NET_ALEN] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static const uint8_t group_mac[NET_ALEN] = {0x03,0x00,0x00,0x00,0x00,0x00};


// FreeRTOS timer callback. Send group beacon.
static void expire_actions(TimerHandle_t pt)
{
	packet_t *item;
	// fixed size packet
	UBaseType_t ret = xRingbufferSendAcquire(sbuf_h, (void**)&item,
		sizeof(packet_t)+sizeof(group_id), pdMS_TO_TICKS(TWAIT));
	if (ret != pdTRUE) return; // drop packet
	memcpy(item->addr, broadcast_mac, NET_ALEN);
	item->pay.type = P_GROUP;
	memcpy(item->pay.data, (void*)&group_id, sizeof(group_id));
	xRingbufferSendComplete(sbuf_h, item); // ignore any errors
}

// ESP-NOW send callback, called from high-priority Wi-Fi task.
static void send_cb(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{
	if (status != ESP_NOW_SEND_SUCCESS) {
		ESP_LOGE(TAG, "Send (in callback) fail:%d to "MACSTR, status, MAC2STR(tx_info->des_addr));
	}
	xTaskNotifyGive(task_h);
}

// ESP-NOW receive callback, called from high-priority Wi-Fi task.
static void recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
	if (len < sizeof(payload_t)) return;
#ifdef DEBUG
	ESP_LOGI(TAG, "recv_cb() len:%d from "MACSTR, len, MAC2STR(peer.peer_addr));
#endif
	if (((payload_t*)data)->type == P_DATA) {
		packet_t *item;
		UBaseType_t ret = xRingbufferSendAcquire(rbuf_h, (void**)&item, len+NET_ALEN, 0);
		if (ret != pdTRUE) return; // drop packet
		memcpy(item->addr, recv_info->src_addr, NET_ALEN);
		memcpy(&item->pay, data, len);
		xRingbufferSendComplete(rbuf_h, item);
	} else if (group_mode && len == sizeof(payload_t)+sizeof(group_id)
		&& memcmp(((payload_t*)data)->data,(void*)&group_id,sizeof(group_id)) == 0) {
		if (esp_now_is_peer_exist(recv_info->src_addr)) return;
		// Add peer to group list
		esp_now_peer_info_t peer; // 36 bytes
		memset(&peer, 0, sizeof(esp_now_peer_info_t));
		memcpy(peer.peer_addr, recv_info->src_addr, NET_ALEN);
		peer.channel = DEFAULT_CHANNEL;
		peer.ifidx = WIFI_IF_STA;
		peer.encrypt = false;
		esp_err_t ret = esp_now_add_peer(&peer);
		if (ret != ESP_OK) {
			ESP_LOGE(TAG, "Add peer fail:%d for "MACSTR, ret, MAC2STR(peer.peer_addr));
		}
#ifdef DEBUG
		else {
			ESP_LOGI(TAG, "esp_now_add_peer("MACSTR")", MAC2STR(peer.peer_addr));
		}
#endif
	}
}

// Process packets in send buffer. Wait for send callback.
static void net_task(void *pvParameter)
{
	uint8_t dst[NET_ALEN];

	xTaskNotifyGive(task_h); // Send first packet without waiting
	for (;;) {
		size_t item_sz;
		packet_t *item = xRingbufferReceive(sbuf_h, &item_sz, portMAX_DELAY);
		if (!item) continue;
		memcpy(dst, item->addr, NET_ALEN);
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait for send callback
		// Note: Sending from app_main freezes. Needs 2+ KiB of stack.
		esp_err_t ret = esp_now_send(IS_GROUP(dst) ? NULL : dst,
			(uint8_t*)&item->pay, item_sz-NET_ALEN);
		if (ret != ESP_OK) {
			xTaskNotifyGive(task_h); // Won't get a send callback
			ESP_LOGE(TAG, "Send fail:%d to "MACSTR, ret, MAC2STR(dst));
		}
#ifdef DEBUG
		else {
			ESP_LOGI(TAG, "esp_now_send("MACSTR")", MAC2STR(dst));
		}
#endif
		vRingbufferReturnItem(sbuf_h, item);
	}
}

// Initialize the network.
// Return zero if successful, or non-zero otherwise.
int32_t net_init(void)
{
	if (timer_h) return ERR_NONE; // return if already initialized.

	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	nvs_flash_f = true;

	// Start Wi-Fi before using ESP-NOW
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(esp_wifi_set_channel(DEFAULT_CHANNEL, WIFI_SECOND_CHAN_NONE));
#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
	ESP_ERROR_CHECK(esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR));
#endif
	esp_wifi_f = true;

	// Create the receive and send buffers.
	rbuf_h = xRingbufferCreate(RINGBUF_SIZE, RINGBUF_TYPE_NOSPLIT);
	if (rbuf_h == NULL) {
		ESP_LOGE(TAG, "Receive ring buffer create fail");
		net_deinit();
		return ERR_FAIL;
	}
	sbuf_h = xRingbufferCreate(RINGBUF_SIZE, RINGBUF_TYPE_NOSPLIT);
	if (sbuf_h == NULL) {
		ESP_LOGE(TAG, "Send ring buffer create fail");
		net_deinit();
		return ERR_FAIL;
	}

	// Initialize ESP-NOW, register callback functions
	ESP_ERROR_CHECK(esp_now_init());
	ESP_ERROR_CHECK(esp_now_register_send_cb(send_cb));
	ESP_ERROR_CHECK(esp_now_register_recv_cb(recv_cb));
	esp_now_f = true;

	// Add broadcast address to group list.
	// When sending broadcast without adding peer,
	// esp_now_send() returns ESP_ERR_ESPNOW_NOT_FOUND : peer is not found
	esp_now_peer_info_t peer; // 36 bytes
	memset(&peer, 0, sizeof(esp_now_peer_info_t));
	memcpy(peer.peer_addr, broadcast_mac, NET_ALEN);
	peer.channel = DEFAULT_CHANNEL;
	peer.ifidx = WIFI_IF_STA;
	peer.encrypt = false;
	ESP_ERROR_CHECK(esp_now_add_peer(&peer));

	// Create the send task. Reads from the send buffer.
	if (xTaskCreate(net_task, "net_task", STACK_SZ, NULL, PRIORITY, &task_h) != pdPASS) {
		ESP_LOGE(TAG, "Task create fail");
		net_deinit();
		return ERR_FAIL;
	}

	// Create the timer for sending group beacons.
	timer_h = xTimerCreate(
		TAG,                    // Text name for the timer.
		pdMS_TO_TICKS(TPERIOD), // The timer period in ticks.
		pdTRUE,                 // Auto reload timer.
		NULL,                   // No need for a timer ID.
		expire_actions          // Function called when timer expires.
	);
	if (timer_h == NULL) {
		ESP_LOGE(TAG, "Timer create fail");
		net_deinit();
		return ERR_FAIL;
	}

	return ERR_NONE;
}

// Free resources used by the network.
// Return zero if successful, or non-zero otherwise.
int32_t net_deinit(void)
{
	if (timer_h) {
		xTimerDelete(timer_h, pdMS_TO_TICKS(TWAIT));
		timer_h = NULL;
	}
	if (task_h) {
		vTaskDelete(task_h); // Stop net_task
		task_h = NULL;
	}
	if (esp_now_f) {
		esp_now_deinit();
		esp_now_f = false;
	}
	if (rbuf_h) {
		vRingbufferDelete(rbuf_h);
		rbuf_h = NULL;
	}
	if (sbuf_h) {
		vRingbufferDelete(sbuf_h);
		sbuf_h = NULL;
	}
	if (esp_wifi_f) {
		esp_wifi_stop(); // Stop Wi-Fi before deinit
		esp_wifi_deinit();
		esp_wifi_f = false;
	}
	if (nvs_flash_f) {
		nvs_flash_deinit();
		nvs_flash_f = false;
	}
	return ERR_NONE;
}

// Send data to the network.
// *dst: pointer to destination address (receiver). If NULL send to group.
// *buf: pointer to data buffer.
// size: size of data in bytes to write.
// wait: time to wait in ms for completion.
// Return number of bytes written, or negative number if error.
int32_t net_send(const uint8_t *dst, const void *buf, uint32_t size, uint32_t wait)
{
	packet_t *item;
	// check for max payload size
	if (size+sizeof(payload_t) > ESP_NOW_MAX_DATA_LEN) return ERR_SIZE;
	UBaseType_t ret = xRingbufferSendAcquire(sbuf_h, (void**)&item, size+sizeof(packet_t), pdMS_TO_TICKS(wait));
	if (ret != pdTRUE) return ERR_RB_GET; // drop packet
	memcpy(item->addr, (dst != NULL) ? dst : group_mac, NET_ALEN);
	item->pay.type = P_DATA;
	memcpy(item->pay.data, buf, size);
	ret = xRingbufferSendComplete(sbuf_h, item);
	if (ret != pdTRUE) return ERR_RB_RET;
	return size;
}

// Receive data from the network.
// *src: pointer to source address (sender).
// *buf: pointer to data buffer.
// size: size of buffer in bytes (maximum allowed to read).
// wait: time to wait in ms for completion.
// Return number of bytes read, or negative number if error.
int32_t net_recv(uint8_t *src, void *buf, uint32_t size, uint32_t wait)
{
	int32_t ret = ERR_NONE;
	size_t item_sz;
	packet_t *item = xRingbufferReceive(rbuf_h, &item_sz, pdMS_TO_TICKS(wait));
	if (!item) return ERR_RB_GET;
	if (item_sz >= sizeof(packet_t)) {
		size_t len = item_sz-sizeof(packet_t);
		if (len > size) len = size;
		memcpy(src, item->addr, NET_ALEN);
		memcpy(buf, item->pay.data, len);
		ret = len;
	} else ret = ERR_SIZE;
	vRingbufferReturnItem(rbuf_h, item);
	return ret;
}

// Open registration for a group, non blocking.
// id: id of the group.
// Return zero on success, or negative number if error.
int32_t net_group_open(uint32_t id)
{
	group_mode = true;
	group_id = id;
	if (xTimerStart(timer_h, pdMS_TO_TICKS(TWAIT)) != pdPASS) {
		return ERR_FAIL;
	}
	return ERR_NONE;
}

// Close registration for a group, non blocking.
// Return zero on success, or negative number if error.
int32_t net_group_close(void)
{
	group_mode = false;
	if (xTimerStop(timer_h, pdMS_TO_TICKS(TWAIT)) != pdPASS) {
		return ERR_FAIL;
	}
	return ERR_NONE;
}

// Clear registration for a group, non blocking.
// Return zero on success, or negative number if error.
int32_t net_group_clear(void)
{
	for (uint8_t i = 0; ; i++) {
		esp_now_peer_info_t peer;
		esp_err_t ret = esp_now_fetch_peer(i==0, &peer);
		if (ret != ESP_OK) break;
		// ESP_LOGI(TAG, "esp_now_fetch_peer() "MACSTR, MAC2STR(peer.peer_addr));
		ret = esp_now_del_peer(peer.peer_addr);
		if (ret != ESP_OK) return ERR_PEER;
	}
	return ERR_NONE;
}

// Return count of peers in the group, or negative number if error.
int32_t net_group_count(void)
{
	esp_now_peer_num_t num;
	esp_err_t ret = esp_now_get_peer_num(&num);
	return (ret != ESP_OK) ? ERR_PEER : num.total_num-1; // ignore broadcast peer
}
