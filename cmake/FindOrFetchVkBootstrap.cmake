# Find vk-bootstrap or fetch latest from GitHub
# Creates vk-bootstrap::vk-bootstrap target
#

include(FetchContent)

# Try to find system vk-bootstrap first
find_package(vk-bootstrap QUIET)

if(vk-bootstrap_FOUND AND TARGET vk-bootstrap::vk-bootstrap)
    message(STATUS "Found system vk-bootstrap")
    set(VK_BOOTSTRAP_SOURCE "System")
else()
    message(STATUS "vk-bootstrap not found on system, fetching latest from GitHub...")
    
    # Fetch latest vk-bootstrap from GitHub
    FetchContent_Declare(
        vk-bootstrap
        GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap.git
        GIT_TAG        origin/main  # Latest commits
        GIT_SHALLOW    FALSE
        SYSTEM         TRUE
    )
    
    FetchContent_GetProperties(vk-bootstrap)
    if(NOT vk-bootstrap_POPULATED)
        message(STATUS "Downloading and configuring vk-bootstrap...")
        
        # Configure vk-bootstrap build options
        set(VK_BOOTSTRAP_DISABLE_WARNINGS ON CACHE BOOL "Disable vk-bootstrap warnings" FORCE)
        set(VK_BOOTSTRAP_WERROR OFF CACHE BOOL "Don't treat warnings as errors" FORCE)
        
        FetchContent_MakeAvailable(vk-bootstrap)
        
        # Set folder for IDE organization
        if(TARGET vk-bootstrap)
            set_target_properties(vk-bootstrap PROPERTIES FOLDER "ThirdParty")
        endif()
        
        message(STATUS "vk-bootstrap configured successfully from latest main")
    endif()
    
    # Create alias if it doesn't exist
    if(TARGET vk-bootstrap AND NOT TARGET vk-bootstrap::vk-bootstrap)
        add_library(vk-bootstrap::vk-bootstrap ALIAS vk-bootstrap)
    endif()
    
    set(VK_BOOTSTRAP_SOURCE "FetchContent (Latest Main)")
endif()

message(STATUS "vk-bootstrap source: ${VK_BOOTSTRAP_SOURCE}")