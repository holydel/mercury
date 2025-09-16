# Mercury Engine Test Bed

A test bed project that demonstrates and tests the Mercury Engine functionality.

## Project Structure

```
mercury/
├── engine/           # Mercury Engine library
│   ├── include/      # Engine headers
│   ├── src/          # Engine source files
│   └── CMakeLists.txt
├── testbed.cpp       # Test bed application
├── CMakeLists.txt    # Main project configuration
├── build.bat         # Windows build script
├── build.sh          # Unix build script
└── README.md         # This file
```

## Building the Test Bed

### Windows
```bash
# Run the build script
build.bat

# Or manually
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Linux/macOS
```bash
# Make the build script executable and run it
chmod +x build.sh
./build.sh

# Or manually
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Running the Test Bed

After building, the executable will be located at:
- Windows: `build/bin/Release/testbed.exe`
- Linux/macOS: `build/bin/testbed`

Run the executable to see the test bed in action. It will:
1. Initialize the Mercury Engine
2. Display platform information
3. Run a simulation loop for 5 seconds
4. Show performance statistics
5. Clean up and exit

## Features Demonstrated

- **Engine Integration**: Links against the Mercury Engine library
- **Platform Detection**: Shows which platform the engine is running on
- **Application Lifecycle**: Demonstrates initialize/tick/shutdown cycle
- **Performance Monitoring**: Tracks frame count and timing
- **Type System**: Uses Mercury Engine's custom type definitions

## Development

The test bed inherits from `mercury::MercuryApplication` and demonstrates:
- Proper engine initialization
- Main loop implementation
- Resource cleanup
- Platform-specific features

## Requirements

- CMake 3.20 or higher
- C++20 compatible compiler
- Visual Studio 2022 (Windows) or GCC/Clang (Linux/macOS)