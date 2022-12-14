cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Pull in Pico SDK (must be before project) (environment variable PICO_SDK_PATH)
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)
project(swx
    VERSION 1.0
    HOMEPAGE_URL https://github.com/saawsm/swx 
    LANGUAGES C CXX ASM
)

# Init the Raspberry Pi Pico SDK
pico_sdk_init()

# ------------------------------------------------------------------------------

add_executable(${PROJECT_NAME})

pico_set_program_name(${PROJECT_NAME} ${PROJECT_NAME})
pico_set_program_version(${PROJECT_NAME} ${PROJECT_VERSION})

# Add source files.
target_sources(${PROJECT_NAME}
    PRIVATE
        "src/main.cpp"
)

# Link the pico standard library 
target_link_libraries(${PROJECT_NAME} PRIVATE pico_stdlib)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        hardware_spi
        hardware_i2c
        hardware_pio
        hardware_adc
        pico_multicore
)

pico_set_program_name(${PROJECT_NAME} ${PROJECT_NAME})
pico_set_program_version(${PROJECT_NAME} ${PROJECT_VERSION})

pico_enable_stdio_uart(${PROJECT_NAME} 1)
pico_enable_stdio_usb(${PROJECT_NAME} 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})


target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wno-format # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
)


