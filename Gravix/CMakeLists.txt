cmake_minimum_required(VERSION 3.20)
project(Gravix)

add_library(Gravix STATIC)

target_precompile_headers(Gravix PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source/pch.h)

if(MSVC)
    target_compile_options(Gravix PRIVATE /utf-8)
endif()

file(GLOB_RECURSE SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/**.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/**.cpp"
)
target_sources(Gravix PRIVATE ${SRC_FILES})

find_package(Vulkan REQUIRED)

target_include_directories(Gravix 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/
        ${CMAKE_SOURCE_DIR}/ThirdParties/spdlog/include
        ${CMAKE_SOURCE_DIR}/ThirdParties/glm
    PRIVATE
        ${Vulkan_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/ThirdParties/VkBootstrap/src
        ${CMAKE_SOURCE_DIR}/ThirdParties/VulkanMemoryAllocator/include
)

target_link_libraries(Gravix
    PRIVATE
        ${Vulkan_LIBRARIES}
)
