cmake_minimum_required(VERSION 3.20)
project(mercury_engine)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/application.cpp
)

# OS specific source files
if(WIN32)
    list(APPEND SOURCES src/ll/os/win32/win32_os.cpp 
    src/ll/os/win32/win32_utils.cpp )
elseif(APPLE)
    list(APPEND SOURCES src/ll/os/macos/macos_os.cpp)
elseif(UNIX AND NOT APPLE)
    list(APPEND SOURCES src/ll/os/linux/linux_os.cpp
    src/ll/os/linux/linux_utils.cpp) 
elseif(EMSCRIPTEN)
    list(APPEND SOURCES src/ll/os/emscripten/emscripten_os.cpp)
elseif(ANDROID)
    list(APPEND SOURCES src/ll/os/android/android_os.cpp)
else()
    message(FATAL_ERROR "Unsupported OS")
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
)

target_include_directories(mercury_engine PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/generated
)

# Set output directories
set_target_properties(mercury_engine PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)
