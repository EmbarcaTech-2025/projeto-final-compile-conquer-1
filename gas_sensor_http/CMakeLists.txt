# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Set any variables required for importing libraries
if (DEFINED ENV{FREERTOS_PATH})
  SET(FREERTOS_PATH $ENV{FREERTOS_PATH})
else()
  SET(FREERTOS_PATH ${CMAKE_CURRENT_LIST_DIR}/FreeRTOS-Kernel)
endif()

message("FreeRTOS Kernel located in ${FREERTOS_PATH}")

# Import those libraries
include(pico_sdk_import.cmake)
include(${FREERTOS_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(gas_sensor_http C CXX ASM)


# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Habilita o servidor DNS da biblioteca Mongoose
add_definitions(-DMG_ENABLE_DNS_SERVER=1)

# Add executable. Default name is the project name, version 0.1
add_executable(${PROJECT_NAME}
        main.c
        ./mongoose-minimal/mongoose.c
        )

# Define as macros necessárias para compilação
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    MG_ARCH=MG_ARCH_PICO
    MG_ENABLE_DNS_SERVER=1  # <-- Adicione esta linha se não existir



)

pico_set_program_name(gas_sensor_http "gas_sensor_http")
pico_set_program_version(gas_sensor_http "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(gas_sensor_http 0)
pico_enable_stdio_usb(gas_sensor_http 1)


# Add the standard include files to the build
target_include_directories(gas_sensor_http PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include 
        ${CMAKE_CURRENT_LIST_DIR}/mongoose-minimal
)

# Add any user requested libraries
target_link_libraries(gas_sensor_http 
        pico_stdlib
        hardware_i2c
        pico_cyw43_arch_lwip_sys_freertos
        FreeRTOS-Kernel-Heap4
        pico_mbedtls
        hardware_pwm
        )

pico_add_extra_outputs(gas_sensor_http)

