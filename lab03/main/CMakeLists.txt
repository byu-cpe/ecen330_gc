idf_component_register(SRCS main.c watch.c
                       INCLUDE_DIRS .
                       PRIV_REQUIRES driver config lcd pin)
# target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
