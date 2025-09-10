# Find Slang shader language or fetch latest from GitHub
# Creates Slang::Slang target
#

include(FetchContent)

# Try to find system Slang first (unlikely to be found)
find_path(SLANG_INCLUDE_DIR slang.h)
find_library(SLANG_LIBRARY NAMES slang libslang)

if(SLANG_INCLUDE_DIR AND SLANG_LIBRARY AND EXISTS "${SLANG_LIBRARY}")
    message(STATUS "Found system Slang: ${SLANG_LIBRARY}")
    
    add_library(Slang::Slang UNKNOWN IMPORTED)
    set_target_properties(Slang::Slang PROPERTIES
        IMPORTED_LOCATION "${SLANG_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SLANG_INCLUDE_DIR}"
    )
    
    set(SLANG_SOURCE "System")
else()
    message(STATUS "Slang not found on system, fetching latest from GitHub...")
    
    # Fetch latest Slang from GitHub
    FetchContent_Declare(
        slang
        GIT_REPOSITORY https://github.com/shader-slang/slang.git
        GIT_TAG        origin/master  # Latest commits
        GIT_SHALLOW    FALSE
        SYSTEM         TRUE
    )
    
    FetchContent_GetProperties(slang)
    if(NOT slang_POPULATED)
        message(STATUS "Downloading and configuring Slang...")
        
        # Configure Slang build options
        set(SLANG_ENABLE_TESTS OFF CACHE BOOL "Enable Slang tests" FORCE)
        set(SLANG_ENABLE_EXAMPLES OFF CACHE BOOL "Enable Slang examples" FORCE)
        set(SLANG_ENABLE_GFX OFF CACHE BOOL "Enable Slang GFX" FORCE)
        set(SLANG_ENABLE_SLANGRT OFF CACHE BOOL "Enable Slang runtime" FORCE)
        set(SLANG_ENABLE_SLANGD OFF CACHE BOOL "Disable slangd (language server) to remove ImGui")
        set(SLANG_ENABLE_SLANGRT OFF CACHE BOOL "Disable runtime target")
        
        FetchContent_MakeAvailable(slang)
        
        # Set folder for IDE organization
        if(TARGET slang)
            set_target_properties(slang PROPERTIES FOLDER "ThirdParty/Slang")
            
            # Create alias if it doesn't exist
            if(NOT TARGET Slang::Slang)
                add_library(Slang::Slang ALIAS slang)
            endif()
        endif()
        
        # Also check for slang-llvm target
        if(TARGET slang-llvm)
            set_target_properties(slang-llvm PROPERTIES FOLDER "ThirdParty/Slang")
        endif()
        
        # Check for additional Slang targets
        if(TARGET slangc)
            set_target_properties(slangc PROPERTIES FOLDER "ThirdParty/Slang")
        endif()
        
        message(STATUS "Slang configured successfully from latest master")
    endif()
    
    set(SLANG_SOURCE "FetchContent (Latest Master)")
endif()

message(STATUS "Slang source: ${SLANG_SOURCE}")