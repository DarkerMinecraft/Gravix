set(RENDERER_SOURCES
   
)

set(RENDERER_HEADERS

)

# Vulkan implementation
if(ENGINE_BUILD_VULKAN)
    find_package(Vulkan REQUIRED)
    
    list(APPEND RENDERER_SOURCES
         src/Vulkan/VulkanDevice.cpp
    )

    list(APPEND RENDERER_HEADERS
        include/Vulkan/VulkanDevice.h
    )
endif()

# D3D11 implementation (Windows only)
if(ENGINE_BUILD_D3D12 AND ENGINE_PLATFORM_WINDOWS)
    list(APPEND RENDERER_SOURCES
        
    )
endif()

add_library(GravixRenderer STATIC ${RENDERER_SOURCES} ${RENDERER_HEADERS})
add_library(Gravix::Renderer ALIAS GravixRenderer)

target_include_directories(GravixRenderer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(GravixRenderer
    PUBLIC
        Gravix::Core
        Gravix::Platform
)


if(ENGINE_BUILD_VULKAN)
    target_link_libraries(GravixRenderer PRIVATE Vulkan::Vulkan)
    target_compile_definitions(GravixRenderer PUBLIC ENGINE_VULKAN_ENABLED)
endif()

if(ENGINE_BUILD_D3D12 AND ENGINE_PLATFORM_WINDOWS)
    target_link_libraries(GravixRenderer PRIVATE d3d11 dxgi)
    target_compile_definitions(GravixRenderer PUBLIC ENGINE_D3D11_ENABLED)
endif()