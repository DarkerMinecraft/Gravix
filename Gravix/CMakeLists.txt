cmake_minimum_required(VERSION 3.16)
project(Gravix)

add_library(Gravix STATIC)

target_precompile_headers(Gravix PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/pch.h)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

target_compile_definitions(Gravix PRIVATE 
    RESOURCES_PATH="$<IF:$<BOOL:${PRODUCTION_BUILD}>,./Resources/,${CMAKE_CURRENT_SOURCE_DIR}/Resources/>"
)

file(GLOB_RECURSE SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/**.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/**.cpp"
)

target_sources(Gravix PRIVATE ${SRC_FILES})

target_link_libraries(Gravix PRIVATE 
    GravixWindows 
    GravixVulkan
)