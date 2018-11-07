cmake_minimum_required(VERSION 3.5)

project(cpparg-test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (${CMAKE_CXX_COMPILER_ID} MATCHES  "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wpedantic")
elseif (${CMAKE_CXX_COMPILER_ID} MATCHES  "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wpedantic")
elseif (${CMAKE_CXX_COMPILER_ID} MATCHES  "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

file(
    GLOB SOURCES
    main.cpp
)

file(
    GLOB HEADERS
    cpparg.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ${HEADERS})
