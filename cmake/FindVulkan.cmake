# Find Vulkan SDK
#
# This module defines:
#   Vulkan_FOUND - True if Vulkan SDK is found
#   Vulkan_INCLUDE_DIRS - Include directories for Vulkan headers
#   Vulkan_LIBRARIES - Libraries to link against
#   Vulkan_VERSION - Version of Vulkan found
#   Vulkan::Vulkan - Imported target for Vulkan
#
# Environment variables used:
#   VULKAN_SDK - Path to Vulkan SDK installation

# First, try to use the built-in FindVulkan if available (CMake 3.7+)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.7")
    include(${CMAKE_ROOT}/Modules/FindVulkan.cmake OPTIONAL RESULT_VARIABLE VULKAN_FOUND_BUILTIN)
    if(VULKAN_FOUND_BUILTIN AND Vulkan_FOUND)
        return()
    endif()
endif()

# Manual Vulkan finding for older CMake versions or custom installations
set(Vulkan_FOUND FALSE)

# Look for Vulkan SDK environment variable
if(DEFINED ENV{VULKAN_SDK})
    set(VULKAN_ROOT_DIR $ENV{VULKAN_SDK})
else()
    # Try common installation paths
    if(WIN32)
        set(VULKAN_SEARCH_PATHS
            "C:/VulkanSDK"
            "C:/SDK/VulkanSDK"
        )
        # Find the most recent version
        file(GLOB VULKAN_VERSIONS RELATIVE ${VULKAN_SEARCH_PATHS} "${VULKAN_SEARCH_PATHS}/*")
        if(VULKAN_VERSIONS)
            list(SORT VULKAN_VERSIONS)
            list(REVERSE VULKAN_VERSIONS)
            list(GET VULKAN_VERSIONS 0 VULKAN_LATEST_VERSION)
            set(VULKAN_ROOT_DIR "${VULKAN_SEARCH_PATHS}/${VULKAN_LATEST_VERSION}")
        endif()
    elseif(UNIX AND NOT APPLE)
        set(VULKAN_SEARCH_PATHS
            "/usr"
            "/usr/local"
            "/opt/vulkan"
            "$ENV{HOME}/VulkanSDK"
        )
    elseif(APPLE)
        set(VULKAN_SEARCH_PATHS
            "/usr/local"
            "/opt/vulkan"
            "$ENV{HOME}/VulkanSDK"
        )
    endif()
endif()

# Find Vulkan header
find_path(Vulkan_INCLUDE_DIRS
    NAMES vulkan/vulkan.h
    PATHS
        ${VULKAN_ROOT_DIR}/Include
        ${VULKAN_ROOT_DIR}/include
        ${VULKAN_SEARCH_PATHS}
    PATH_SUFFIXES
        include
        Include
    DOC "Vulkan include directory"
)

# Find Vulkan library
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(VULKAN_LIB_SUFFIX "Lib")
    else()
        set(VULKAN_LIB_SUFFIX "Lib32")
    endif()
    
    find_library(Vulkan_LIBRARIES
        NAMES vulkan-1 vulkan
        PATHS
            ${VULKAN_ROOT_DIR}/${VULKAN_LIB_SUFFIX}
            ${VULKAN_ROOT_DIR}/lib
        DOC "Vulkan library"
    )
else()
    find_library(Vulkan_LIBRARIES
        NAMES vulkan
        PATHS
            ${VULKAN_ROOT_DIR}/lib
            ${VULKAN_SEARCH_PATHS}
        PATH_SUFFIXES
            lib
            lib64
        DOC "Vulkan library"
    )
endif()

# Try to extract version from vulkan_core.h
if(Vulkan_INCLUDE_DIRS)
    set(VULKAN_CORE_H "${Vulkan_INCLUDE_DIRS}/vulkan/vulkan_core.h")
    if(EXISTS "${VULKAN_CORE_H}")
        file(READ "${VULKAN_CORE_H}" VULKAN_CORE_CONTENTS)
        
        string(REGEX MATCH "#define VK_HEADER_VERSION_COMPLETE VK_MAKE_API_VERSION\\(0, ([0-9]+), ([0-9]+), VK_HEADER_VERSION\\)" 
               VULKAN_VERSION_MATCH "${VULKAN_CORE_CONTENTS}")
        
        if(CMAKE_MATCH_1 AND CMAKE_MATCH_2)
            set(Vulkan_VERSION_MAJOR ${CMAKE_MATCH_1})
            set(Vulkan_VERSION_MINOR ${CMAKE_MATCH_2})
            
            # Get patch version
            string(REGEX MATCH "#define VK_HEADER_VERSION ([0-9]+)" 
                   VULKAN_PATCH_MATCH "${VULKAN_CORE_CONTENTS}")
            if(CMAKE_MATCH_1)
                set(Vulkan_VERSION_PATCH ${CMAKE_MATCH_1})
            else()
                set(Vulkan_VERSION_PATCH 0)
            endif()
            
            set(Vulkan_VERSION "${Vulkan_VERSION_MAJOR}.${Vulkan_VERSION_MINOR}.${Vulkan_VERSION_PATCH}")
        endif()
    endif()
endif()

# Standard CMake package handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Vulkan
    REQUIRED_VARS Vulkan_LIBRARIES Vulkan_INCLUDE_DIRS
    VERSION_VAR Vulkan_VERSION
)

# Create imported target
if(Vulkan_FOUND AND NOT TARGET Vulkan::Vulkan)
    add_library(Vulkan::Vulkan UNKNOWN IMPORTED)
    set_target_properties(Vulkan::Vulkan PROPERTIES
        IMPORTED_LOCATION "${Vulkan_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIRS}"
    )
    
    # Add platform-specific link libraries
    if(WIN32)
        # No additional libraries needed on Windows
    elseif(UNIX AND NOT APPLE)
        # Linux may need additional libraries
        find_library(X11_LIBRARIES X11)
        find_library(XCB_LIBRARIES xcb)
        if(X11_LIBRARIES)
            set_property(TARGET Vulkan::Vulkan APPEND PROPERTY 
                         INTERFACE_LINK_LIBRARIES ${X11_LIBRARIES})
        endif()
        if(XCB_LIBRARIES)
            set_property(TARGET Vulkan::Vulkan APPEND PROPERTY 
                         INTERFACE_LINK_LIBRARIES ${XCB_LIBRARIES})
        endif()
    elseif(APPLE)
        # macOS may need additional frameworks
        find_library(COCOA_LIBRARY Cocoa)
        find_library(IOKIT_LIBRARY IOKit)
        find_library(COREVIDEO_LIBRARY CoreVideo)
        if(COCOA_LIBRARY)
            set_property(TARGET Vulkan::Vulkan APPEND PROPERTY 
                         INTERFACE_LINK_LIBRARIES ${COCOA_LIBRARY})
        endif()
        if(IOKIT_LIBRARY)
            set_property(TARGET Vulkan::Vulkan APPEND PROPERTY 
                         INTERFACE_LINK_LIBRARIES ${IOKIT_LIBRARY})
        endif()
        if(COREVIDEO_LIBRARY)
            set_property(TARGET Vulkan::Vulkan APPEND PROPERTY 
                         INTERFACE_LINK_LIBRARIES ${COREVIDEO_LIBRARY})
        endif()
    endif()
endif()

# Mark advanced variables
mark_as_advanced(Vulkan_INCLUDE_DIRS Vulkan_LIBRARIES)

# Print status
if(Vulkan_FOUND)
    message(STATUS "Found Vulkan: ${Vulkan_LIBRARIES} (version ${Vulkan_VERSION})")
else()
    if(Vulkan_FIND_REQUIRED)
        message(FATAL_ERROR "Could not find Vulkan SDK. Please set VULKAN_SDK environment variable.")
    else()
        message(STATUS "Vulkan SDK not found. Set VULKAN_SDK environment variable if needed.")
    endif()
endif()