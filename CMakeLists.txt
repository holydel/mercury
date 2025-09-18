cmake_minimum_required(VERSION 3.20)
project(mercury_testbed)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Link Time Optimization (LTO) - but only for native builds
if(NOT EMSCRIPTEN)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_output)
    if(ipo_supported)
        message(STATUS "IPO/LTO is supported")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(WARNING "IPO/LTO is not supported: ${ipo_output}")
    endif()

    # Platform-specific LTO flags - only for native builds
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # GCC and Clang LTO flags
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
        set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} -flto")
        set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} -flto")
          
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        # MSVC LTO flags
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /GL")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
        set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG")
        set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG")
    endif()

    if(MSVC)
        add_compile_options(/arch:SSE4.1)
    else()
        add_compile_options(-msse4.1)
    endif()
else()
    message(STATUS "Configuring Emscripten build with WebGPU support")
    
    # Enable WebAssembly SIMD for Emscripten
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msimd128")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msimd128")
    message(STATUS "Enabled WebAssembly SIMD support (-msimd128)")
    
    # WebGPU port configuration
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --use-port=emdawnwebgpu")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --use-port=emdawnwebgpu")
    
    # Emscripten-specific linker flags for WebGPU
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ALLOW_MEMORY_GROWTH=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXPORT_ES6=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s MODULARIZE=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ASYNCIFY=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s FETCH=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --use-port=emdawnwebgpu")

    message(STATUS "Emscripten WebGPU configuration complete")
endif()

# Add subdirectories
add_subdirectory(engine)

add_subdirectory(editor)
# Test bed executable
add_executable(testbed 
    testbed.cpp
)

# Link against mercury engine
if(EMSCRIPTEN)
    target_link_libraries(testbed PRIVATE mercury_engine)
else()
    target_link_libraries(testbed mercury_engine)
endif()

# Include directories for test bed
target_include_directories(testbed PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/include
)

# Set output directories
set_target_properties(testbed PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Emscripten-specific target properties
if(EMSCRIPTEN)
    set_target_properties(testbed PROPERTIES
        SUFFIX ".html"
        OUTPUT_NAME "testbed"
    )
    
    # Set HTML output name
    set_target_properties(testbed PROPERTIES
        OUTPUT_NAME_DEBUG "testbed"
        OUTPUT_NAME_RELEASE "testbed"
    )
endif()
