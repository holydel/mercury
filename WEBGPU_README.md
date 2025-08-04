# Mercury Engine WebGPU Setup

This document explains how to build and run the Mercury Engine with WebGPU support using Emscripten and the emdawnwebgpu backend.

## Overview

The Mercury Engine now supports WebGPU through Emscripten, providing:
- **WebGPU Graphics API** - Modern GPU programming for the web
- **emdawnwebgpu Backend** - Google's Dawn WebGPU implementation
- **HTML Shell Output** - Custom web interface with WebGPU detection
- **SIMD Support** - WebAssembly SIMD for optimal performance

## Prerequisites

### 1. Emscripten SDK
Install and activate the latest Emscripten SDK:

```bash
# Clone emsdk
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk

# Install latest version
./emsdk install latest
./emsdk activate latest

# Set up environment (Windows)
emsdk_env.bat

# Set up environment (Linux/macOS)
source ./emsdk_env.sh
```

### 2. WebGPU-Enabled Browser
WebGPU is still experimental and requires specific browser versions:

#### Chrome/Chromium
- **Version**: 113+ (Chrome Canary recommended)
- **Enable Flag**: `chrome://flags/#enable-unsafe-webgpu`
- **Status**: Set to "Enabled"

#### Firefox
- **Version**: Nightly build
- **Config**: `about:config` → `webgpu.enabled` = `true`

#### Safari
- **Version**: 16.4+ (limited support)
- **Status**: Experimental support

## Building with WebGPU Support

### Option 1: Using Build Script (Recommended)

```bash
# Windows
./build-emsdk-webgpu.bat

# Linux/macOS
./build-emsdk-webgpu.sh
```

### Option 2: Using CMake Presets

```bash
# Configure with WebGPU preset
cmake --preset emscripten-webgpu

# Build
cmake --build --preset emscripten-webgpu
```

### Option 3: Manual CMake

```bash
# Create build directory
mkdir build-emscripten-webgpu
cd build-emscripten-webgpu

# Configure
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DEMSCRIPTEN=1 ..

# Build
cmake --build . --config Release
```

## Generated Files

After a successful build, you'll find these files in `build-emscripten-webgpu/bin/`:

- **`testbed.js`** - WebAssembly module with WebGPU bindings
- **`testbed.html`** - Web page with WebGPU interface
- **`testbed.wasm`** - WebAssembly binary
- **`testbed.wasm.map`** - Source map for debugging

## Running the WebGPU Application

### 1. Start Local Web Server

```bash
# Python 3
python -m http.server 8000

# Python 2
python -m SimpleHTTPServer 8000

# Node.js
npx http-server -p 8000
```

### 2. Open in Browser

Navigate to: `http://localhost:8000/bin/testbed.html`

### 3. Verify WebGPU Support

The web interface will automatically detect and display:
- **WebGPU Support**: Browser capability status
- **SIMD Support**: WebAssembly SIMD availability
- **Performance**: Engine status

## WebGPU Configuration

### Emscripten Linker Flags

The build system automatically configures these Emscripten flags:

```bash
# HTML shell and WebGPU support
--shell-file=shell.html
-s USE_WEBGPU=1
-s USE_DAWN=1

# Memory and performance
-s ALLOW_MEMORY_GROWTH=1
-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']

# ES6 module support
-s EXPORT_ES6=1
-s MODULARIZE=1
-s EXPORT_NAME='MercuryEngine'

# WebGPU features
-s USE_WEBGPU2=1
```

### Custom HTML Shell

The `shell.html` file provides:
- **Modern UI** - Glassmorphism design with WebGPU detection
- **Canvas Integration** - WebGPU-compatible canvas setup
- **Engine Controls** - Start/stop/reset functionality
- **Fullscreen Support** - Native fullscreen API integration
- **Responsive Design** - Mobile-friendly interface

## WebGPU API Integration

### C++ WebGPU Headers

The engine includes WebGPU C++ headers from [Google's Dawn project](https://github.com/google/dawn):

```cpp
#include <webgpu/webgpu_cpp.h>

// WebGPU device and adapter setup
wgpu::Instance instance = wgpu::CreateInstance();
wgpu::Adapter adapter = instance.RequestAdapter();
wgpu::Device device = adapter.RequestDevice();
```

### JavaScript Integration

The HTML shell provides JavaScript APIs for WebGPU:

```javascript
// WebGPU feature detection
function checkWebGPUSupport() {
    if (navigator.gpu) {
        return navigator.gpu.getPreferredCanvasFormat ? 'Supported' : 'Limited';
    }
    return 'Not Supported';
}

// Engine module loading
window.MercuryEngine().then(module => {
    module._main(); // Call C++ main function
});
```

## Performance Considerations

### WebGPU Optimizations

1. **Memory Management**
   - `ALLOW_MEMORY_GROWTH=1` for dynamic memory allocation
   - Efficient WebAssembly memory usage

2. **SIMD Performance**
   - WebAssembly SIMD enabled with `-msimd128`
   - Template-based lane extraction for optimal performance

3. **Module Loading**
   - ES6 module support for better tree-shaking
   - Modularized WebAssembly for faster loading

### Browser Performance

- **Chrome**: Best WebGPU performance, full feature support
- **Firefox**: Good performance, experimental features
- **Safari**: Limited support, basic functionality

## Troubleshooting

### Common Issues

#### 1. WebGPU Not Available
```
Error: WebGPU not supported in this browser
```
**Solution**: Enable WebGPU flags in browser settings

#### 2. Emscripten Build Failures
```
Error: USE_WEBGPU=1 not found
```
**Solution**: Update to latest Emscripten SDK

#### 3. Module Loading Errors
```
Error: Failed to load WebAssembly module
```
**Solution**: Serve files through HTTP server (not file://)

#### 4. Canvas Context Issues
```
Error: WebGPU context creation failed
```
**Solution**: Check browser WebGPU support and flags

### Debug Mode

Enable debug output by setting environment variables:

```bash
# Windows
set EMCC_DEBUG=1
set EMCC_VERBOSE=1

# Linux/macOS
export EMCC_DEBUG=1
export EMCC_VERBOSE=1
```

## Development Workflow

### 1. Development Build
```bash
cmake --preset emscripten-webgpu
cmake --build --preset emscripten-webgpu --config Debug
```

### 2. Production Build
```bash
cmake --preset emscripten-webgpu
cmake --build --preset emscripten-webgpu --config Release
```

### 3. Hot Reload
Use a development server with auto-reload:
```bash
npx live-server --port=8000 --open=/bin/testbed.html
```

## References

- [Emscripten WebGPU Documentation](https://emscripten.org/docs/porting/graphics/webgpu.html)
- [Google Dawn WebGPU Implementation](https://github.com/google/dawn)
- [WebGPU Specification](https://gpuweb.github.io/gpuweb/)
- [WebAssembly SIMD](https://webassembly.org/simd/)

## Browser Compatibility Matrix

| Browser | Version | WebGPU | SIMD | Performance |
|---------|---------|--------|------|-------------|
| Chrome | 113+ | ✅ Full | ✅ | Excellent |
| Firefox | Nightly | ✅ Full | ✅ | Good |
| Safari | 16.4+ | ⚠️ Limited | ✅ | Basic |
| Edge | 113+ | ✅ Full | ✅ | Excellent |

## Next Steps

1. **Implement WebGPU Graphics Pipeline** - Add actual WebGPU rendering
2. **Add WebGPU Compute Shaders** - Utilize GPU compute capabilities
3. **Optimize for Mobile** - Improve mobile WebGPU performance
4. **Add WebGPU Debugging** - Implement WebGPU validation layers 