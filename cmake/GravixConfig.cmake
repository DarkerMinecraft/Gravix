# Game Engine Configuration
# Sets up common configuration, compiler flags, and helper functions
#

# Prevent in-source builds
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed. Please create a build directory.")
endif()

# Set default build type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Debug' as none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Ensure output directories exist
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(MAKE_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
file(MAKE_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})

# Platform detection macros
macro(engine_detect_platform)
    if(WIN32)
        set(ENGINE_PLATFORM_WINDOWS TRUE)
        set(ENGINE_PLATFORM_NAME "Windows")
        message(STATUS "Platform: Windows")
    elseif(UNIX AND NOT APPLE)
        set(ENGINE_PLATFORM_LINUX TRUE)
        set(ENGINE_PLATFORM_NAME "Linux")
        message(STATUS "Platform: Linux")
    elseif(APPLE)
        set(ENGINE_PLATFORM_MACOS TRUE)
        set(ENGINE_PLATFORM_NAME "macOS")
        message(STATUS "Platform: macOS")
    else()
        message(WARNING "Unknown platform detected")
        set(ENGINE_PLATFORM_NAME "Unknown")
    endif()
endmacro()

# Compiler detection and setup
macro(engine_setup_compiler)
    # Detect compiler
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(ENGINE_COMPILER_MSVC TRUE)
        set(ENGINE_COMPILER_NAME "MSVC")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(ENGINE_COMPILER_GCC TRUE)
        set(ENGINE_COMPILER_NAME "GCC")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(ENGINE_COMPILER_CLANG TRUE)
        set(ENGINE_COMPILER_NAME "Clang")
    endif()
    
    message(STATUS "Compiler: ${ENGINE_COMPILER_NAME} ${CMAKE_CXX_COMPILER_VERSION}")
    
    # Common compiler flags
    if(ENGINE_COMPILER_MSVC)
        # MSVC specific flags
        add_compile_options(
            /W4                     # Warning level 4
            /WX-                    # Don't treat warnings as errors by default
            /permissive-            # Disable non-conforming code
            /Zc:__cplusplus         # Enable correct __cplusplus macro
            /utf-8                  # Set source and execution character sets to UTF-8
        )
        
        # Debug flags
        add_compile_options($<$<CONFIG:Debug>:/Od>)      # No optimization
        add_compile_options($<$<CONFIG:Debug>:/Zi>)      # Debug information
        add_compile_options($<$<CONFIG:Debug>:/RTC1>)    # Runtime checks
        
        # Release flags
        add_compile_options($<$<CONFIG:Release>:/O2>)    # Optimize for speed
        add_compile_options($<$<CONFIG:Release>:/Oi>)    # Enable intrinsic functions
        add_compile_options($<$<CONFIG:Release>:/GL>)    # Whole program optimization
        
        # Link flags for release
        add_link_options($<$<CONFIG:Release>:/LTCG>)     # Link-time code generation
        
    else()
        # GCC/Clang flags
        add_compile_options(
            -Wall                   # Enable common warnings
            -Wextra                 # Enable extra warnings
            -Wpedantic              # Enable pedantic warnings
            -Wno-unused-parameter   # Disable unused parameter warnings
        )
        
        # Debug flags
        add_compile_options($<$<CONFIG:Debug>:-g>)       # Debug information
        add_compile_options($<$<CONFIG:Debug>:-O0>)      # No optimization
        add_compile_options($<$<CONFIG:Debug>:-fno-omit-frame-pointer>)
        
        # Release flags
        add_compile_options($<$<CONFIG:Release>:-O3>)    # Optimize for speed
        add_compile_options($<$<CONFIG:Release>:-DNDEBUG>) # Disable asserts
        add_compile_options($<$<CONFIG:Release>:-ffast-math>) # Fast math
        
        # Additional GCC-specific flags
        if(ENGINE_COMPILER_GCC)
            add_compile_options($<$<CONFIG:Release>:-flto>) # Link-time optimization
        endif()
    endif()
endmacro()

# Function to add engine module with common settings
function(engine_add_module MODULE_NAME)
    # Parse arguments
    set(options INTERFACE SHARED)
    set(oneValueArgs FOLDER)
    set(multiValueArgs SOURCES HEADERS DEPENDENCIES SYSTEM_DEPENDENCIES)
    cmake_parse_arguments(MODULE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # Determine library type
    if(MODULE_INTERFACE)
        set(LIB_TYPE INTERFACE)
    elseif(MODULE_SHARED)
        set(LIB_TYPE SHARED)
    else()
        set(LIB_TYPE STATIC)
    endif()
    
    # Create the library
    add_library(${MODULE_NAME} ${LIB_TYPE} ${MODULE_SOURCES} ${MODULE_HEADERS})
    add_library(Gravix::${MODULE_NAME} ALIAS ${MODULE_NAME})
    
    # Set properties
    if(NOT MODULE_INTERFACE)
        set_target_properties(${MODULE_NAME} PROPERTIES
            CXX_STANDARD 23
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
            VERSION ${PROJECT_VERSION}
            SOVERSION ${PROJECT_VERSION_MAJOR}
        )
        
        # Set folder for IDE organization
        if(MODULE_FOLDER)
            set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${MODULE_FOLDER})
        endif()
    endif()
    
    # Set include directories
    target_include_directories(${MODULE_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    # Link dependencies
    if(MODULE_DEPENDENCIES)
        target_link_libraries(${MODULE_NAME} PUBLIC ${MODULE_DEPENDENCIES})
    endif()
    
    if(MODULE_SYSTEM_DEPENDENCIES)
        target_link_libraries(${MODULE_NAME} PRIVATE ${MODULE_SYSTEM_DEPENDENCIES})
    endif()
    
    # Add compile definitions
    target_compile_definitions(${MODULE_NAME}
        PUBLIC
            $<$<CONFIG:Debug>:ENGINE_DEBUG>
            $<$<CONFIG:Release>:ENGINE_RELEASE>
            $<$<BOOL:${ENGINE_PLATFORM_WINDOWS}>:ENGINE_PLATFORM_WINDOWS>
            $<$<BOOL:${ENGINE_PLATFORM_LINUX}>:ENGINE_PLATFORM_LINUX>
            $<$<BOOL:${ENGINE_PLATFORM_MACOS}>:ENGINE_PLATFORM_MACOS>
    )
    
    message(STATUS "Added engine module: ${MODULE_NAME} (${LIB_TYPE})")
endfunction()

# Function to setup precompiled headers for a target
function(engine_setup_pch TARGET_NAME PCH_HEADER)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
        target_precompile_headers(${TARGET_NAME} PRIVATE ${PCH_HEADER})
        message(STATUS "Precompiled headers enabled for ${TARGET_NAME}")
    else()
        message(WARNING "CMake 3.16+ required for precompiled headers. Skipping for ${TARGET_NAME}")
    endif()
endfunction()

# Function to copy assets to output directory
function(engine_copy_assets TARGET_NAME ASSETS_DIR)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${ASSETS_DIR} $<TARGET_FILE_DIR:${TARGET_NAME}>/Assets
        COMMENT "Copying assets for ${TARGET_NAME}"
    )
endfunction()

# Function to setup debugging
function(engine_setup_debugging TARGET_NAME)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        # Add debug-specific settings
        if(ENGINE_PLATFORM_WINDOWS AND ENGINE_COMPILER_MSVC)
            # Enable debug heap on Windows
            target_compile_definitions(${TARGET_NAME} PRIVATE _CRTDBG_MAP_ALLOC)
        endif()
        
        # Add sanitizers on GCC/Clang
        if(ENGINE_COMPILER_GCC OR ENGINE_COMPILER_CLANG)
            option(ENABLE_SANITIZERS "Enable sanitizers in debug builds" OFF)
            if(ENABLE_SANITIZERS)
                target_compile_options(${TARGET_NAME} PRIVATE
                    -fsanitize=address
                    -fsanitize=undefined
                    -fno-sanitize-recover=all
                )
                target_link_options(${TARGET_NAME} PRIVATE
                    -fsanitize=address
                    -fsanitize=undefined
                )
                message(STATUS "Sanitizers enabled for ${TARGET_NAME}")
            endif()
        endif()
    endif()
endfunction()

# Setup initial configuration
engine_detect_platform()
engine_setup_compiler()

# Print configuration summary
message(STATUS "=== Game Engine Configuration ===")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Platform: ${ENGINE_PLATFORM_NAME}")
message(STATUS "Compiler: ${ENGINE_COMPILER_NAME}")
message(STATUS "C++ Standard: 23")
message(STATUS "Output Directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "==================================")