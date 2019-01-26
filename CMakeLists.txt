cmake_minimum_required(VERSION 3.5)

project(cpparg)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

file(GLOB HEADERS
    include/cpparg/cpparg.h
)

add_library(${PROJECT_NAME} INTERFACE)
add_library(cpparg::cpparg ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME} INTERFACE
    ${HEADERS}
)

target_include_directories(
    ${PROJECT_NAME} INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
)

option(CPPARG_BUILD_TESTS "Build tests" OFF)
if (${CPPARG_BUILD_TESTS})
    enable_testing()
    add_subdirectory(test)
endif()

option(CPPARG_BUILD_EXAMPLES "Build examples" OFF)
if (${CPPARG_BUILD_EXAMPLES})
    add_subdirectory(examples)
endif()
