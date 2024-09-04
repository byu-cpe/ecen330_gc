idf_component_register(SRCS main.c
                       INCLUDE_DIRS .
                       PRIV_REQUIRES config lcd pin tone joy c24k_8b)
# target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
