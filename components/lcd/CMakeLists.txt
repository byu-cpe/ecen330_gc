idf_component_register(SRCS lcd.c
                       INCLUDE_DIRS .
                       PRIV_REQUIRES esp_driver_gpio esp_driver_spi
                       REQUIRES config)
# target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
