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


project(Fall_Detection C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

set(MAIN_TARGET_NAME ${PROJECT_NAME})

file(GLOB SRC_FILES
        "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/*.c"
)

# Add executable. Default name is the project name, version 0.1
add_executable(${MAIN_TARGET_NAME}
        main.cpp
        ${SRC_FILES}
        ./mongoose-minimal/mongoose.c
        )

add_subdirectory(model)

pico_set_program_name(${MAIN_TARGET_NAME} "${MAIN_TARGET_NAME}")
pico_set_program_version(${MAIN_TARGET_NAME} "3.0.0")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(${MAIN_TARGET_NAME} 0)
pico_enable_stdio_usb(${MAIN_TARGET_NAME} 1)

# Add the standard library to the build
target_link_libraries(${MAIN_TARGET_NAME}
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(${MAIN_TARGET_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include 
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/mongoose-minimal
)

# Add any user requested libraries
target_link_libraries(${MAIN_TARGET_NAME} 
        hardware_i2c
        hardware_pwm
        pico_cyw43_arch_lwip_sys_freertos
        FreeRTOS-Kernel-Heap4
        pico_mbedtls
        )

pico_add_extra_outputs(${MAIN_TARGET_NAME})

