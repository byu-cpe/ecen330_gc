idf_component_register(SRCS main.c game.c missile.c plane.c
                       INCLUDE_DIRS .
                       PRIV_REQUIRES esp_timer config lcd cursor pin sound c24k_8b)
# target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
