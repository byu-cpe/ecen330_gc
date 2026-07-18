idf_component_register(SRCS main.c
                       INCLUDE_DIRS .
                       PRIV_REQUIRES esp_driver_gpio esp_timer config lcd joy tone)
# target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
