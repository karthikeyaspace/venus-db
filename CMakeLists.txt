cmake_minimum_required(VERSION 3.10)
project(VenusDB CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

include_directories(
    ${PROJECT_SOURCE_DIR}
)
    
file(GLOB_RECURSE SOURCES 
    "${PROJECT_SOURCE_DIR}/binder/*.cpp"
    "${PROJECT_SOURCE_DIR}/buffer/*.cpp" 
    "${PROJECT_SOURCE_DIR}/catalog/*.cpp"
    "${PROJECT_SOURCE_DIR}/common/*.cpp"
    "${PROJECT_SOURCE_DIR}/database/*.cpp"
    "${PROJECT_SOURCE_DIR}/engine/*.cpp"
    "${PROJECT_SOURCE_DIR}/executor/*.cpp"
    "${PROJECT_SOURCE_DIR}/parser/*.cpp"
    "${PROJECT_SOURCE_DIR}/planner/*.cpp"
    "${PROJECT_SOURCE_DIR}/storage/*.cpp"
    "${PROJECT_SOURCE_DIR}/table/*.cpp"
    "${PROJECT_SOURCE_DIR}/main.cpp"
)

add_executable(venus ${SOURCES})

if (MINGW)
    target_link_libraries(venus PRIVATE -static-libgcc -static-libstdc++ -lpthread)
endif()
