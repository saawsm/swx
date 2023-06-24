if (ESP_PLATFORM) # Support using swx as an ESP-IDF component for access to protocol & misc header files

    idf_component_register(INCLUDE_DIRS "include")
    
else() # configure as swx driver firmware using pico-sdk

    cmake_minimum_required(VERSION 3.20)

    # Development / debugging options
    set(USB_WAIT_TIME_MS 0 CACHE STRING "Delay in milliseconds to wait for USB connection (Defaults OFF, or 15 seconds in Debug builds)")

    set(STARTUP_CAL_ENABLED 0b1111 CACHE STRING "Channels that will be calibrated at startup. Bit flag: LSB is 1st channel, MSB is last channel")
    option(IGNORE_CAL_ERRORS "Ignore channel calibration errors. Warning: Could damage hardware!" OFF)

    option(I2C_CHECK_WRITE "Check if the I2C write buffer is full before every I2C write" OFF)

    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    # Add the board directory so the pico-sdk can find it
    list(APPEND PICO_BOARD_HEADER_DIRS ${CMAKE_SOURCE_DIR}/boards)

    # Pull in Pico SDK (must be before project) (environment variable PICO_SDK_PATH)
    include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

    project(swx
        VERSION 1.0
        HOMEPAGE_URL https://github.com/saawsm/swx 
        LANGUAGES C CXX ASM
    )

    # Init the Raspberry Pi Pico SDK
    pico_sdk_init()

    add_executable(${PROJECT_NAME})

    # Add public headers.
    target_include_directories(${PROJECT_NAME} PRIVATE "./include/swx")

    # Add source files.
    target_sources(${PROJECT_NAME}
        PRIVATE
            "src/main.c"
            "src/output.c"
            "src/channel.c"
            "src/pulse_gen.c"
            "src/protocol.c"
            "src/audio.c"
            "src/analog_capture.c"
            "src/util/i2c.c"        
            "src/hardware/mcp4728.c"
            "src/hardware/ads1015.c"
    )

    # Generate PIO headers (place them in the build/generated folder)
    pico_generate_pio_header(${PROJECT_NAME} ../src/pio/pulse_gen.pio OUTPUT_DIR build/generated)

    # Link the pico standard library 
    target_link_libraries(${PROJECT_NAME} PRIVATE pico_stdlib)

    # Link libraries
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            hardware_i2c
            hardware_pio
            hardware_adc
            hardware_dma
            pico_multicore
            pico_i2c_slave
    )

    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Wno-format # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
    )

    pico_set_program_name(${PROJECT_NAME} ${PROJECT_NAME})
    pico_set_program_version(${PROJECT_NAME} ${PROJECT_VERSION})

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        pico_enable_stdio_usb(${PROJECT_NAME} 1)
        pico_enable_stdio_uart(${PROJECT_NAME} 1)
    endif()

    # pico_set_float_implementation(${PROJECT_NAME} pico)
    # pico_set_binary_type(${PROJECT_NAME} no_flash)

    # Create map/bin/hex/uf2 files
    pico_add_extra_outputs(${PROJECT_NAME})

    # -----------------------------------------------------------------------------------------------------------------

    target_compile_definitions(${PROJECT_NAME} PRIVATE CH_CAL_ENABLED=${STARTUP_CAL_ENABLED})

    if(IGNORE_CAL_ERRORS)
        message("Ignoring channel calibration errors!")
        target_compile_definitions(${PROJECT_NAME} PRIVATE CH_IGNORE_CAL_ERRORS)
    endif()

    if(I2C_CHECK_WRITE)
        target_compile_definitions(${PROJECT_NAME} PRIVATE I2C_CHECK_WRITE)
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT USB_WAIT_TIME_MS)
        message("Set USB_WAIT_TIME_MS to 15 seconds, since debug")
        set (USB_WAIT_TIME_MS 15000)
    endif()

    if(USB_WAIT_TIME_MS)
        target_compile_definitions(${PROJECT_NAME} PRIVATE PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=${USB_WAIT_TIME_MS})
    endif()
endif()