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

if(ENGINE_USE_VULKAN)
    add_compile_definitions(Gravix PRIVATE ENGINE_USE_VULKAN)
endif()

target_include_directories(Gravix 
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/

        ${CMAKE_SOURCE_DIR}/ThirdParties/spdlog/include
        ${CMAKE_SOURCE_DIR}/ThirdParties/glm
        ${CMAKE_SOURCE_DIR}/ThirdParties/ImGui
        ${CMAKE_SOURCE_DIR}/ThirdParties/ImGuizmo
        ${CMAKE_SOURCE_DIR}/ThirdParties/entt/single_include
    PRIVATE
        ${Vulkan_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/ThirdParties/VkBootstrap/src
        ${CMAKE_SOURCE_DIR}/ThirdParties/VulkanMemoryAllocator/include
        ${CMAKE_SOURCE_DIR}/ThirdParties/enkiTS/src
        ${CMAKE_SOURCE_DIR}/ThirdParties/Slang/include
        ${CMAKE_SOURCE_DIR}/ThirdParties/stb/
        ${CMAKE_SOURCE_DIR}/ThirdParties/yaml/include
)

target_link_libraries(Gravix
    PRIVATE
        ${Vulkan_LIBRARIES}
        vk-bootstrap
        enkiTS
        ImGui
        ImGuizmo
        slang
        yaml-cpp
    PUBLIC
        GravixScripting
)
