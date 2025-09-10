# Find spdlog or fetch latest from GitHub
# Creates spdlog::spdlog target
#

include(FetchContent)

# Try to find system spdlog first
find_package(spdlog QUIET)

if(spdlog_FOUND AND TARGET spdlog::spdlog)
    message(STATUS "Found system spdlog: ${spdlog_VERSION}")
    set(SPDLOG_SOURCE "System")
else()
    message(STATUS "spdlog not found on system, fetching latest from GitHub...")
    
    # Fetch latest spdlog from GitHub
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        origin/v1.x  # Latest stable v1.x branch
        GIT_SHALLOW    FALSE
        SYSTEM         TRUE
    )
    
    FetchContent_GetProperties(spdlog)
    if(NOT spdlog_POPULATED)
        message(STATUS "Downloading and configuring spdlog...")
        
        # Configure spdlog build options
        set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "Build spdlog examples" FORCE)
        set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "Build spdlog tests" FORCE)
        set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "Build spdlog benchmarks" FORCE)
        set(SPDLOG_FMT_EXTERNAL OFF CACHE BOOL "Use external fmt library" FORCE)
        set(SPDLOG_INSTALL ON CACHE BOOL "Generate install target" FORCE)
        
        # Performance options
        set(SPDLOG_ENABLE_PCH ON CACHE BOOL "Enable precompiled headers" FORCE)
        set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "Build shared library" FORCE)
        
        FetchContent_MakeAvailable(spdlog)
        
        # Set folder for IDE organization
        if(TARGET spdlog)
            set_target_properties(spdlog PROPERTIES FOLDER "ThirdParty")
        endif()
        
        message(STATUS "spdlog configured successfully from latest v1.x")
    endif()
    
    set(SPDLOG_SOURCE "FetchContent (Latest v1.x)")
endif()

message(STATUS "spdlog source: ${SPDLOG_SOURCE}")