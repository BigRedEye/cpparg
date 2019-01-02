cmake_minimum_required(VERSION 3.9)

project(cpparg-test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(DownloadGoogleTest)
include(Sanitizers)

file(GLOB SOURCES
    main.cpp
    parser.cpp
    util.cpp
    args_builder.cpp
)

file(GLOB HEADERS
    args_builder.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} PUBLIC cpparg gtest_main gtest)

add_test(UnitTest ${PROJECT_NAME})
