# Master file to fetch all engine dependencies

message(STATUS "=== Fetching Engine Dependencies ===")

# Set common FetchContent options
set(FETCHCONTENT_QUIET OFF)  # Show download progress
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)  # Don't update every configure

# Include all dependency finders
include(${CMAKE_SOURCE_DIR}/cmake/FindOrFetchImGui.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/FindOrFetchVMA.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/FindOrFetchVkBootstrap.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/FindOrFetchSpdlog.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/FindOrFetchSlang.cmake)

# Optional: Include audio if enabled
if(ENGINE_BUILD_AUDIO)
    include(${CMAKE_SOURCE_DIR}/cmake/FindOrFetchOpenAL.cmake)
endif()

message(STATUS "=== All Dependencies Configured ===")

# Helper function to create ImGui backend targets
function(add_imgui_backend TARGET_NAME BACKEND_SOURCES)
    if(NOT TARGET imgui::imgui)
        message(FATAL_ERROR "ImGui must be available before creating backend ${TARGET_NAME}")
    endif()
    
    add_library(${TARGET_NAME} STATIC ${BACKEND_SOURCES})
    
    target_link_libraries(${TARGET_NAME}
        PUBLIC
            imgui::imgui
    )
    
    target_include_directories(${TARGET_NAME}
        PUBLIC
            ${IMGUI_BACKENDS_DIR}
    )
    
    set_target_properties(${TARGET_NAME} PROPERTIES
        FOLDER "ThirdParty/ImGui"
        POSITION_INDEPENDENT_CODE ON
    )
    
    message(STATUS "Created ImGui backend: ${TARGET_NAME}")
endfunction()