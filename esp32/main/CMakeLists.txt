idf_component_register(
    SRCS
        "paperframe.c" "djmWifi.c"
        "epd_esp32.c"
        "../../components/epd/epd.c"
        "../../components/png/png.c"
    INCLUDE_DIRS
        "../../components"
)

include(${CMAKE_CURRENT_LIST_DIR}/private.cmake)
set_source_files_properties(paperframe.c
    PROPERTIES COMPILE_FLAGS
    -DWIFIPASS=\\"${WIFIPASS}\\"
)
