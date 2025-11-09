# ============================================================================
# Vulkan Configuration
# ============================================================================

if(NOT GRAVIX_USE_VULKAN)
    return()
endif()

find_package(Vulkan REQUIRED)

if(Vulkan_FOUND)
    message(STATUS "Vulkan found: ${Vulkan_VERSION}")
    message(STATUS "  Include: ${Vulkan_INCLUDE_DIRS}")
    message(STATUS "  Library: ${Vulkan_LIBRARIES}")

    add_compile_definitions(ENGINE_USE_VULKAN)
else()
    message(FATAL_ERROR "Vulkan SDK not found! Please install the Vulkan SDK.")
endif()
