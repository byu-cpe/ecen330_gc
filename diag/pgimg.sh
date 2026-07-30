#!/bin/bash
# Usage: ./pgimg.sh <input.img>
esptool write-flash 0x0 $1
