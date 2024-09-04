idf_component_register(SRCS main.c lcd_test.c crosshair.c peppers.c
                       INCLUDE_DIRS .
                       PRIV_REQUIRES lcd esp_timer)
# target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
