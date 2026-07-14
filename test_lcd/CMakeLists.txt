# Do not change the order of these commands
cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS ../components)
set(COMPONENTS main)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(lcd_test)
# idf_build_set_property(COMPILE_OPTIONS "-Wno-error" APPEND)

# Create a SPIFFS image from the contents of the 'font' directory
# that fits the partition named 'storage'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash
# spiffs_create_partition_image(storage ../font FLASH_IN_PROJECT)
