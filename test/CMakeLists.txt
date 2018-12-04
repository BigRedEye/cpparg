cmake_minimum_required(VERSION 3.5)

project(cpparg-test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
    
find_package(GTest REQUIRED)

file(GLOB SOURCES
    main.cpp
    parser.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PUBLIC cpparg ${GTEST_BOTH_LIBRARIES})
target_include_directories(${PROJECT_NAME} PUBLIC ${GTEST_INCLUDE_DIRS}) 
