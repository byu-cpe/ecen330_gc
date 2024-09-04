idf_component_register(SRCS main.c gameControl.c missile.c plane.c
                       INCLUDE_DIRS .
                       PRIV_REQUIRES config esp_timer lcd pin cursor sound c24k_8b)
# target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
