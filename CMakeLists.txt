cmake_minimum_required(VERSION 3.10)
project(VenusDB CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


# Linux environment
set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")

set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

include_directories(
    ${PROJECT_SOURCE_DIR}
)
    
file(GLOB_RECURSE SOURCES 
    "${PROJECT_SOURCE_DIR}/storage/*.cpp"
    "${PROJECT_SOURCE_DIR}/buffer/*.cpp" 
    "${PROJECT_SOURCE_DIR}/table/*.cpp"
    "${PROJECT_SOURCE_DIR}/catalog/*.cpp"
    "${PROJECT_SOURCE_DIR}/database/*.cpp"
    "${PROJECT_SOURCE_DIR}/common/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
)

add_executable(venus
    ${SOURCES}
)


# To execute

# cd build
# cmake . .
# make
# ./venus