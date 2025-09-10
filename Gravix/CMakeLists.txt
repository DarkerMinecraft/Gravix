add_subdirectory(Core)
add_subdirectory(Platform)
add_subdirectory(Renderer)

if(ENGINE_BUILD_AUDIO)
    #add_subdirectory(Audio)
endif()

# Create main engine library that links everything
add_library(Gravix INTERFACE)
add_library(Gravix::Engine ALIAS Gravix)

target_link_libraries(Gravix
    INTERFACE
        Gravix::Core
        Gravix::Platform
        Gravix::Renderer
        #$<$<BOOL:${ENGINE_BUILD_AUDIO}>:Engine::Audio>
)