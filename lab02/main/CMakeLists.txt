idf_component_register(SRCS main.c pin_test.c
                       INCLUDE_DIRS .
                       PRIV_REQUIRES esp_driver_gpio config lcd pin)
# target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
