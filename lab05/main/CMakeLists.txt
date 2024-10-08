set(SOURCE main.c game.c board.c graphics.c nav.c)
if(EXISTS ../main/com.c)
    set(MILESTONE 2)
    list(APPEND SOURCE com.c)
else()
    set(MILESTONE 1)
endif()
message(STATUS "MILESTONE=${MILESTONE}")
idf_component_register(SRCS ${SOURCE}
                       INCLUDE_DIRS .
                       PRIV_REQUIRES esp_timer driver config lcd pin joy)
target_compile_options(${COMPONENT_LIB} PRIVATE -DMILESTONE=${MILESTONE})
