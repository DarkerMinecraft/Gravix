cmake_minimum_required(VERSION 3.16)
project(Orbit)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

target_compile_definitions(Gravis PRIVATE 
    RESOURCES_PATH="$<IF:$<BOOL:${PRODUCTION_BUILD}>,./Resources/,${CMAKE_CURRENT_SOURCE_DIR}/Resources/>"
)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB_RECURSE SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/**.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/**.cpp"
)

target_sources(Orbit PRIVATE ${SRC_FILES})

target_link_libraries(Orbit PRIVATE Gravis)
