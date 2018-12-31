cmake_minimum_required(VERSION 3.5)

project(cpparg)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    SET( CMAKE_CXX_FLAGS_SANITIZERS "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=undefined,address -fsanitize-undefined-trap-on-error"
        CACHE STRING "Flags for C++ with UBSan and ASan enabled."
        FORCE)
    SET( CMAKE_C_FLAGS_SANITIZERS "${CMAKE_C_FLAGS_DEBUG} -fsanitize=undefined,address -fsanitize-undefined-trap-on-error"
        CACHE STRING "Flags for C with UBSan and ASan enabled."
        FORCE)
    SET( CMAKE_EXE_LINKER_FLAGS_SANITIZERS
        "${CMAKE_EXE_LINKER_FLAGS_DEBUG}" CACHE STRING
        "Flags for linker with UBSan and ASan enabled."
        FORCE )
    SET( CMAKE_SHARED_LINKER_FLAGS_SANITIZERS
        "${CMAKE_SHARED_LINKER_FLAGS_DEBUG}" CACHE STRING
        "Flags for linker with UBSan and ASan enabled."
        FORCE )
    MARK_AS_ADVANCED(
        CMAKE_CXX_FLAGS_SANITIZERS
        CMAKE_C_FLAGS_SANITIZERS
        CMAKE_EXE_LINKER_FLAGS_SANITIZERS
        CMAKE_SHARED_LINKER_FLAGS_SANITIZERS )
    # Update the documentation string of CMAKE_BUILD_TYPE for GUIs
    SET( CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel Sanitizers."
        FORCE )
endif()

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
