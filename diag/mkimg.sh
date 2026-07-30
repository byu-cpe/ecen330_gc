#!/bin/bash
# Usage: ./mkimg.sh <output.img>
esptool -c esp32 merge-bin -o $1 --flash-mode dio --flash-size 4MB --flash-freq 40m 0x1000 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x10000 build/diag.bin
# https://docs.espressif.com/projects/esptool/en/latest/esp32/esptool/basic-commands.html#merge-binaries-for-flashing-merge-bin
