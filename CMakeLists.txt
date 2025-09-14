cmake_minimum_required(VERSION 3.10)
project(VenusDB CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

include_directories(${PROJECT_SOURCE_DIR})

# core
file(GLOB_RECURSE CORE_SOURCES 
    "${PROJECT_SOURCE_DIR}/binder/*.cpp"
    "${PROJECT_SOURCE_DIR}/buffer/*.cpp" 
    "${PROJECT_SOURCE_DIR}/catalog/*.cpp"
    "${PROJECT_SOURCE_DIR}/common/*.cpp"
    "${PROJECT_SOURCE_DIR}/database/*.cpp"
    "${PROJECT_SOURCE_DIR}/engine/*.cpp"
    "${PROJECT_SOURCE_DIR}/executor/*.cpp"
    "${PROJECT_SOURCE_DIR}/network/*.cpp"
    "${PROJECT_SOURCE_DIR}/parser/*.cpp"
    "${PROJECT_SOURCE_DIR}/planner/*.cpp"
    "${PROJECT_SOURCE_DIR}/storage/*.cpp"
    "${PROJECT_SOURCE_DIR}/table/*.cpp"
)

add_library(venus_core STATIC ${CORE_SOURCES})

add_executable(venus "${PROJECT_SOURCE_DIR}/main.cpp")
target_link_libraries(venus venus_core)

option(BUILD_TESTS "Build test executable" OFF)

if(BUILD_TESTS)
    add_executable(test "tests/load_insert_test.cpp")
    target_link_libraries(test venus_core)
endif()

if (MINGW)
    target_link_libraries(venus PRIVATE -static-libgcc -static-libstdc++ -lpthread)
    if(BUILD_TESTS)
        target_link_libraries(test PRIVATE -static-libgcc -static-libstdc++ -lpthread)
    endif()
endif()
