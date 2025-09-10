set(AUDIO_SOURCES
    
)

set(AUDIO_HEADERS
    
)

add_library(GravixAudio STATIC ${AUDIO_SOURCES} ${AUDIO_HEADERS})
add_library(Gravix::Audio ALIAS GravixAudio)

target_include_directories(GravixAudio
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(GravixAudio
    PUBLIC
        Engine::Core
    PRIVATE
        OpenAL::OpenAL
)