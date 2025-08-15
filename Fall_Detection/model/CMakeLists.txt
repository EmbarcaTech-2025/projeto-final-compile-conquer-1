cmake_minimum_required(VERSION 3.13.1)

if(NOT DEFINED MAIN_TARGET_NAME)
    message(FATAL_ERROR "MAIN_TARGET_NAME must be set before including this file")
endif()

if(NOT TARGET ${MAIN_TARGET_NAME})
    message(FATAL_ERROR "Target '${MAIN_TARGET_NAME}' does not exist")
endif()

include(edge-impulse-sdk/cmake/utils.cmake)
add_subdirectory(edge-impulse-sdk/cmake/zephyr)

RECURSIVE_FIND_FILE_APPEND(MODEL_SOURCE "tflite-model" "*.cpp")
target_include_directories(${MAIN_TARGET_NAME} PRIVATE .)

# add all sources to the project
target_sources(${MAIN_TARGET_NAME} PRIVATE ${MODEL_SOURCE})
