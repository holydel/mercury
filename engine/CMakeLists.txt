cmake_minimum_required(VERSION 3.20)
project(mercury_engine)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Source files
set(SOURCES
    src/application.cpp
    src/mercury_log.cpp    
    src/input.cpp
    src/mercury_memory.cpp
    src/mercury_string_utils.cpp
    #GRAPHICS
    src/graphics.cpp
    src/ll/graphics/null/null_graphics.cpp
    src/ll/graphics/mercury_swapchain.cpp
    #IMGUI
    src/imgui/mercury_imgui.cpp
    src/imgui/imgui_impl.cpp
    ../third_party/imgui/imgui.cpp
    ../third_party/imgui/imgui_draw.cpp
    ../third_party/imgui/imgui_widgets.cpp
    ../third_party/imgui/imgui_tables.cpp
    ../third_party/imgui/imgui_demo.cpp
)

# OS specific source files

set(VULKAN_AVAILABLE FALSE)
set(D3D12_AVAILABLE FALSE)
set(WEBGPU_AVAILABLE FALSE)
set(METAL_AVAILABLE FALSE)

if(EMSCRIPTEN)
    message(STATUS "Autodetected Emscripten build")

    set(WEBGPU_AVAILABLE TRUE)

    list(APPEND SOURCES src/ll/os/emscripten/emscripten_os.cpp
    src/ll/os/emscripten/emscripten_utils.cpp
    src/ll/os/emscripten/emscripten_input.cpp)
elseif(WIN32)
    message(STATUS "Autodetected Windows build")

    set(VULKAN_AVAILABLE TRUE)
    set(D3D12_AVAILABLE TRUE)
    set(WEBGPU_AVAILABLE TRUE)

    list(APPEND SOURCES src/ll/os/win32/win32_os.cpp 
    src/ll/os/win32/win32_utils.cpp
    src/ll/os/win32/win32_input.cpp)


    add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN)

elseif(APPLE)
    message(STATUS "Autodetected macOS build")

    set(VULKAN_AVAILABLE TRUE)
    
    list(APPEND SOURCES src/ll/os/macos/macos_os.cpp)
elseif(UNIX AND NOT APPLE)
    message(STATUS "Autodetected Linux build")

    set(VULKAN_AVAILABLE TRUE)

    list(APPEND SOURCES src/ll/os/linux/linux_os.cpp
    src/ll/os/linux/linux_utils.cpp) 
elseif(ANDROID)
    message(STATUS "Autodetected Android build")
    set(VULKAN_AVAILABLE TRUE)

    list(APPEND SOURCES src/ll/os/android/android_os.cpp)
else()
    message(FATAL_ERROR "Unsupported OS")
endif()

if(VULKAN_AVAILABLE)
    message(STATUS "Vulkan support enabled")
    list(APPEND SOURCES
        src/ll/graphics/vulkan/mercury_vulkan.cpp
        src/ll/graphics/vulkan/vk_utils.cpp
        src/ll/graphics/vulkan/vk_graphics.cpp
        src/ll/graphics/vulkan/vk_swapchain.cpp
        src/ll/graphics/vulkan/vk_instance.cpp
        src/ll/graphics/vulkan/vk_device.cpp
        src/ll/graphics/vulkan/vk_adapter.cpp
        src/ll/graphics/vulkan/vk_swapchain.cpp
    )
endif()


#if(D3D12_AVAILABLE)
#    list(APPEND SOURCES
#        src/ll/graphics/d3d12/mercury_d3d12.cpp
#        src/ll/graphics/d3d12/d3d12_utils.cpp
#    )
#endif()

if(WEBGPU_AVAILABLE)
    list(APPEND SOURCES
        src/ll/graphics/webgpu/webgpu_graphics.cpp
        src/ll/graphics/webgpu/webgpu_utils.cpp
    )
endif()

# Create library
add_library(mercury_engine STATIC ${SOURCES})

message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "Fetching Git commit hash, branch, and date...")
# Find Git
find_package(Git REQUIRED)

# Get latest commit short hash
execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get current branch name
execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH_NAME
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get commit date (ISO format)
execute_process(
    COMMAND ${GIT_EXECUTABLE} log -1 --format=%cd --date=iso
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_DATE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Configure header template
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version-info.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/version-info.h
    @ONLY
)

# Include directories
target_include_directories(mercury_engine PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include

    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/Vulkan-Headers/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/VulkanMemoryAllocator/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/DirectX-Headers/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/D3D12MemoryAllocator/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/imgui
)

target_include_directories(mercury_engine PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/generated
)

# Set output directories
set_target_properties(mercury_engine PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)


