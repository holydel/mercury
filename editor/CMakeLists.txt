cmake_minimum_required(VERSION 3.20)
project(mercury_editor)

add_executable(mercury_editor 
    src/editor_main.cpp
    src/editor_main_window.cpp
)


target_link_libraries(mercury_editor mercury_engine)

# Include directories for test bed
target_include_directories(mercury_editor PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}../engine/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Set output directories
set_target_properties(mercury_editor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)