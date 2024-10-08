#ifndef COM_H_
#define COM_H_

#include <stdint.h>

// This component is a wrapper around a lower-level communication
// method such as a serial port (UART), Bluetooth, or WiFi. The
// complexities of establishing a communication channel and sending
// bytes through the channel are abstracted away. The read and write
// functions are non-blocking so they can be used in a tick function.

// Initialize the communication channel.
// Return zero if successful, or non-zero otherwise.
int32_t com_init(void);

// Free resources used for communication.
// Return zero if successful, or non-zero otherwise.
int32_t com_deinit(void);

// Write data to the communication channel. Does not wait for data.
// *buf: pointer to data buffer
// size: size of data in bytes to write
// Return number of bytes written, or negative number if error.
int32_t com_write(const void *buf, uint32_t size);

// Read data from the communication channel. Does not wait for data.
// *buf: pointer to data buffer
// size: size of data in bytes to read
// Return number of bytes read, or negative number if error.
int32_t com_read(void *buf, uint32_t size);

#endif // COM_H_
