cmake_minimum_required(VERSION 3.13)

include(pico-sdk/pico_sdk_init.cmake)

project(paperframe)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(paperframe
        paperframe.c
        led.c
        serial.c

        epd.c
        usb_descriptors.c

        ../components/png/png.c
        ../components/miniz/miniz.c
)

add_compile_definitions( PICO_BOOTSEL_VIA_DOUBLE_RESET_TIMEOUT_MS=1000 )

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(paperframe pico_stdlib)
target_link_libraries(paperframe hardware_pwm)
target_link_libraries(paperframe pico_stdlib hardware_spi)
target_link_libraries(paperframe pico_bootsel_via_double_reset)
target_link_libraries(paperframe tinyusb_device tinyusb_board)

# so tinyusb can #include our "tusb_config.h" 
target_include_directories( paperframe PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../components
)

# Enable USB, UART output
#pico_enable_stdio_usb(paperframe 0)
#pico_enable_stdio_uart(paperframe 1)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(paperframe)

