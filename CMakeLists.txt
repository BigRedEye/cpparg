cmake_minimum_required(VERSION 3.5)

project(cpparg)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

file(GLOB HEADERS
    include/cpparg/cpparg.h
)

add_library(${PROJECT_NAME} INTERFACE)

target_sources(${PROJECT_NAME} INTERFACE
    ${HEADERS}
)

target_include_directories(
    ${PROJECT_NAME} INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
)

option(BUILD_TESTS "Build tests" OFF)
if (${BUILD_TESTS})
    enable_testing()
    add_subdirectory(test)
endif()

option(BUILD_EXAMPLES "Build examples" OFF)
if (${BUILD_EXAMPLES})
    add_subdirectory(examples)
endif()
