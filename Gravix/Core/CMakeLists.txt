set(CORE_SOURCES
    src/Application.cpp
)

set(CORE_HEADERS
    include/Application.h
)

add_library(GravixCore STATIC ${CORE_SOURCES} ${CORE_HEADERS})
add_library(Gravix::Core ALIAS GravixCore)

target_include_directories(GravixCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(GravixCore PUBLIC cxx_std_20)

# Platform-specific linking
if(ENGINE_PLATFORM_WINDOWS)
    target_link_libraries(GravixCore PRIVATE winmm)
endif()