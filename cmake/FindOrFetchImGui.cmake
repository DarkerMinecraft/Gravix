# Find ImGui or fetch latest from GitHub
# Creates ImGui target, plus compatibility aliases: imgui, imgui::imgui
#
include(FetchContent)

# Try to find system ImGui first
find_package(imgui QUIET)
if(imgui_FOUND AND TARGET imgui::imgui)
    message(STATUS "Found system ImGui")
    set(IMGUI_SOURCE "System")
    # Create alias for consistency
    add_library(ImGui ALIAS imgui::imgui)
else()
    message(STATUS "ImGui not found on system, fetching latest from GitHub...")
    
    # Fetch latest ImGui from GitHub (docking branch has latest features)
    FetchContent_Declare(
        ImGuiSrc
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        docking  # Latest docking branch (most features)
        GIT_SHALLOW    TRUE     # Changed to TRUE for faster downloads
        SYSTEM         TRUE
    )
    
    # This will download and populate the content
    FetchContent_MakeAvailable(ImGuiSrc)
    
    # Now that the content is available, we can safely reference the files
    set(IMGUI_SOURCES
        ${imguisrc_SOURCE_DIR}/imgui.cpp
        ${imguisrc_SOURCE_DIR}/imgui_demo.cpp
        ${imguisrc_SOURCE_DIR}/imgui_draw.cpp
        ${imguisrc_SOURCE_DIR}/imgui_tables.cpp
        ${imguisrc_SOURCE_DIR}/imgui_widgets.cpp
    )
    
    set(IMGUI_HEADERS
        ${imguisrc_SOURCE_DIR}/imgui.h
        ${imguisrc_SOURCE_DIR}/imconfig.h
        ${imguisrc_SOURCE_DIR}/imgui_internal.h
        ${imguisrc_SOURCE_DIR}/imstb_rectpack.h
        ${imguisrc_SOURCE_DIR}/imstb_textedit.h
        ${imguisrc_SOURCE_DIR}/imstb_truetype.h
    )
    
    # Create capitalized ImGui library
    add_library(ImGui STATIC ${IMGUI_SOURCES} ${IMGUI_HEADERS})
    
    target_include_directories(ImGui
        PUBLIC
            $<BUILD_INTERFACE:${imguisrc_SOURCE_DIR}>
            $<INSTALL_INTERFACE:include>
    )
    
    target_compile_features(ImGui PUBLIC cxx_std_11)
    
    set_target_properties(ImGui PROPERTIES
        FOLDER "ThirdParty"
        POSITION_INDEPENDENT_CODE ON
    )
    
    if(WIN32)
        target_compile_definitions(ImGui PUBLIC IMGUI_IMPL_WIN32_DISABLE_GAMEPAD)
    endif()
    
    # Create namespaced alias for compatibility
    add_library(imgui::imgui ALIAS ImGui)
    
    message(STATUS "ImGui configured successfully from latest docking branch")
    set(IMGUI_SOURCE "FetchContent (Latest Docking)")
endif()

# Store backend sources for easy access
if(TARGET imgui::imgui OR TARGET ImGui)
    set(IMGUI_BACKENDS_DIR ${imguisrc_SOURCE_DIR}/backends CACHE PATH "ImGui backends directory")
    
    # Common backend combinations
    set(IMGUI_GLFW_OPENGL3_SOURCES
        ${IMGUI_BACKENDS_DIR}/imgui_impl_glfw.cpp
        ${IMGUI_BACKENDS_DIR}/imgui_impl_opengl3.cpp
        CACHE INTERNAL "ImGui GLFW + OpenGL3 backend sources"
    )
    
    set(IMGUI_WIN32_DX11_SOURCES
        ${IMGUI_BACKENDS_DIR}/imgui_impl_win32.cpp
        ${IMGUI_BACKENDS_DIR}/imgui_impl_dx11.cpp
        CACHE INTERNAL "ImGui Win32 + DX11 backend sources"
    )
    
    set(IMGUI_GLFW_VULKAN_SOURCES
        ${IMGUI_BACKENDS_DIR}/imgui_impl_glfw.cpp
        ${IMGUI_BACKENDS_DIR}/imgui_impl_vulkan.cpp
        CACHE INTERNAL "ImGui GLFW + Vulkan backend sources"
    )
    
    set(IMGUI_WIN32_VULKAN_SOURCES
        ${IMGUI_BACKENDS_DIR}/imgui_impl_win32.cpp
        ${IMGUI_BACKENDS_DIR}/imgui_impl_vulkan.cpp
        CACHE INTERNAL "ImGui Win32 + Vulkan backend sources"
    )
endif()

message(STATUS "ImGui source: ${IMGUI_SOURCE}")