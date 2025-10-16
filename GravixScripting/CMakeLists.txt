cmake_minimum_required(VERSION 3.20)
project(GravixScripting)

add_library(GravixScripting STATIC)

file(GLOB_RECURSE SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/**.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/**.cpp"
)
target_sources(GravixScripting PRIVATE ${SRC_FILES})

if(MSVC)
    target_compile_options(GravixScripting PRIVATE /utf-8)
endif()

target_include_directories(GravixScripting 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/
    PRIVATE
        "C:/Program Files/dotnet/packs/Microsoft.NETCore.App.Host.win-x64/9.0.10/runtimes/win-x64/native/"
)