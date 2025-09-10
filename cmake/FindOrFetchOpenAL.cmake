# Find OpenAL or fetch OpenAL-Soft if not found
# Creates OpenAL::OpenAL target

include(FetchContent)

# First try to find system OpenAL
find_package(OpenAL QUIET)

if(OpenAL_FOUND OR TARGET OpenAL::OpenAL)
    message(STATUS "Found system OpenAL: ${OPENAL_LIBRARY}")
    
    # Ensure we have the modern target
    if(NOT TARGET OpenAL::OpenAL AND OpenAL_FOUND)
        add_library(OpenAL::OpenAL UNKNOWN IMPORTED)
        set_target_properties(OpenAL::OpenAL PROPERTIES
            IMPORTED_LOCATION "${OPENAL_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${OPENAL_INCLUDE_DIR}"
        )
    endif()
    
    set(OPENAL_SOURCE "System")
else()
    message(STATUS "OpenAL not found on system, fetching OpenAL-Soft...")
    
    # Fetch OpenAL-Soft from GitHub
    FetchContent_Declare(
        openal-soft
        GIT_REPOSITORY https://github.com/kcat/openal-soft.git
        GIT_TAG        origin/master
        GIT_SHALLOW    TRUE
    )
    
    # Configure OpenAL-Soft build options
    set(ALSOFT_UTILS OFF CACHE BOOL "Build utility programs" FORCE)
    set(ALSOFT_NO_CONFIG_UTIL ON CACHE BOOL "Disable building the alsoft-config utility" FORCE)
    set(ALSOFT_EXAMPLES OFF CACHE BOOL "Build example programs" FORCE)
    set(ALSOFT_TESTS OFF CACHE BOOL "Build test programs" FORCE)
    set(ALSOFT_INSTALL_CONFIG OFF CACHE BOOL "Install alsoft.conf sample configuration file" FORCE)
    set(ALSOFT_INSTALL_HRTF_DATA OFF CACHE BOOL "Install HRTF data files" FORCE)
    set(ALSOFT_INSTALL_AMBDEC_PRESETS OFF CACHE BOOL "Install AmbDec preset files" FORCE)
    
    # Platform-specific options
    if(WIN32)
        set(ALSOFT_BACKEND_WINMM ON CACHE BOOL "Enable Windows Multimedia backend" FORCE)
        set(ALSOFT_BACKEND_DSOUND ON CACHE BOOL "Enable DirectSound backend" FORCE)
        set(ALSOFT_BACKEND_WASAPI ON CACHE BOOL "Enable WASAPI backend" FORCE)
    elseif(APPLE)
        set(ALSOFT_BACKEND_COREAUDIO ON CACHE BOOL "Enable CoreAudio backend" FORCE)
    else()
        set(ALSOFT_BACKEND_ALSA ON CACHE BOOL "Enable ALSA backend" FORCE)
        set(ALSOFT_BACKEND_PULSEAUDIO ON CACHE BOOL "Enable PulseAudio backend" FORCE)
    endif()
    
    # Disable backends we don't need
    set(ALSOFT_BACKEND_OSS OFF CACHE BOOL "Enable OSS backend" FORCE)
    set(ALSOFT_BACKEND_SOLARIS OFF CACHE BOOL "Enable Solaris backend" FORCE)
    set(ALSOFT_BACKEND_SNDIO OFF CACHE BOOL "Enable SndIO backend" FORCE)
    set(ALSOFT_BACKEND_QSA OFF CACHE BOOL "Enable QSA backend" FORCE)
    set(ALSOFT_BACKEND_PORTAUDIO OFF CACHE BOOL "Enable PortAudio backend" FORCE)
    set(ALSOFT_BACKEND_JACK OFF CACHE BOOL "Enable JACK backend" FORCE)
    
    # Make available
    FetchContent_MakeAvailable(openal-soft)
    
    # The target created by OpenAL-Soft is 'OpenAL' not 'OpenAL::OpenAL'
    # Create an alias for consistency
    if(TARGET OpenAL AND NOT TARGET OpenAL::OpenAL)
        add_library(OpenAL::OpenAL ALIAS OpenAL)
    endif()
    
    # Set folder for IDE organization
    if(TARGET OpenAL)
        set_target_properties(OpenAL PROPERTIES FOLDER "ThirdParty/OpenAL")
    endif()
    
    set(OPENAL_SOURCE "FetchContent (OpenAL-Soft)")
    message(STATUS "OpenAL-Soft fetched and configured successfully")
endif()

message(STATUS "OpenAL source: ${OPENAL_SOURCE}")