# Find Vulkan Memory Allocator or fetch latest from GitHub
# Creates VulkanMemoryAllocator::VulkanMemoryAllocator target
#

include(FetchContent)

# Try to find system VMA first
find_package(VulkanMemoryAllocator QUIET)

if(VulkanMemoryAllocator_FOUND AND TARGET VulkanMemoryAllocator::VulkanMemoryAllocator)
    message(STATUS "Found system Vulkan Memory Allocator")
    set(VMA_SOURCE "System")
else()
    message(STATUS "VMA not found on system, fetching latest from GitHub...")
    
    # Fetch latest VMA from GitHub
    FetchContent_Declare(
        vma
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
        GIT_TAG        origin/master  # Latest commits
        GIT_SHALLOW    FALSE
        SYSTEM         TRUE
    )
    
    FetchContent_GetProperties(vma)
    if(NOT vma_POPULATED)
        message(STATUS "Downloading and configuring VMA...")
        FetchContent_Populate(vma)
        
        # VMA is header-only, but we can create an interface library
        add_library(VulkanMemoryAllocator INTERFACE)
        add_library(VulkanMemoryAllocator::VulkanMemoryAllocator ALIAS VulkanMemoryAllocator)
        
        target_include_directories(VulkanMemoryAllocator
            INTERFACE
                $<BUILD_INTERFACE:${vma_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
        )
        
        # VMA requires Vulkan
        find_package(Vulkan REQUIRED)
        target_link_libraries(VulkanMemoryAllocator INTERFACE Vulkan::Vulkan)
        
        # Set folder for IDE organization
        set_target_properties(VulkanMemoryAllocator PROPERTIES FOLDER "ThirdParty")
        
        # Add compile definitions for VMA configuration
        target_compile_definitions(VulkanMemoryAllocator INTERFACE
            VMA_STATIC_VULKAN_FUNCTIONS=0  # Use dynamic linking
            VMA_DYNAMIC_VULKAN_FUNCTIONS=1
        )
        
        message(STATUS "VMA configured successfully from latest master")
    endif()
    
    set(VMA_SOURCE "FetchContent (Latest Master)")
endif()

message(STATUS "VMA source: ${VMA_SOURCE}")