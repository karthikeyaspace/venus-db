cmake_minimum_required(VERSION 3.10)
project(MiniDB CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


# Linux environment
set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")

set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

include_directories(
    # ${PROJECT_SOURCE_DIR}/catalog
    # ${PROJECT_SOURCE_DIR}/common
    # ${PROJECT_SOURCE_DIR}/executor
    ${PROJECT_SOURCE_DIR}/storage
    # ${PROJECT_SOURCE_DIR}/index
    # ${PROJECT_SOURCE_DIR}/buffer
    # ${PROJECT_SOURCE_DIR}/parser
    # ${PROJECT_SOURCE_DIR}/planner
)

file(GLOB_RECURSE SOURCES 
    "${PROJECT_SOURCE_DIR}/*.cpp"
    "${PROJECT_SOURCE_DIR}/storage/*.cpp"
)

add_executable(minidb
    ${SOURCES}
)


# To execute

# cd build
# cmake . .
# make
# ./minidb