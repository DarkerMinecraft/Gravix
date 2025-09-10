set(PLATFORM_SOURCES
   
)

set(PLATFORM_HEADERS
    
)

# Add platform-specific sources
if(ENGINE_PLATFORM_WINDOWS)
    list(APPEND PLATFORM_SOURCES
        src/Windows/WindowsPlatform.cpp
        src/Windows/WindowsWindow.cpp
        src/Windows/WindowsInput.cpp
    )
    list(APPEND PLATFORM_HEADERS
        src/Windows/WindowsPlatform.h
    )
elseif(ENGINE_PLATFORM_LINUX)
    list(APPEND PLATFORM_SOURCES
        src/Linux/LinuxPlatform.cpp
        src/Linux/LinuxWindow.cpp
        src/Linux/LinuxInput.cpp
    )
endif()

add_library(GravixPlatform STATIC ${PLATFORM_SOURCES} ${PLATFORM_HEADERS})
add_library(Gravix::Platform ALIAS GravixPlatform)

target_include_directories(GravixPlatform
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(GravixPlatform
    PUBLIC
        Gravix::Core
)

# Platform-specific dependencies
if(ENGINE_PLATFORM_WINDOWS)
    target_link_libraries(GravixPlatform PRIVATE user32 gdi32)
elseif(ENGINE_PLATFORM_LINUX)
    find_package(X11 REQUIRED)
    target_link_libraries(GravixPlatform PRIVATE ${X11_LIBRARIES})
    target_include_directories(GravixPlatform PRIVATE ${X11_INCLUDE_DIR})
endif()