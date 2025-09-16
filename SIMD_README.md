# Mercury Engine SIMD Support

This document explains the SIMD (Single Instruction, Multiple Data) implementation in the Mercury Engine, with special focus on Emscripten WebAssembly SIMD support.

## Overview

The Mercury Engine provides cross-platform SIMD support through the following implementations:

1. **ARM NEON** - For ARM processors (mobile devices, Apple Silicon)
2. **x86/x64 SSE** - For Intel/AMD processors (Windows, Linux, macOS)
3. **WebAssembly SIMD** - For Emscripten builds (web browsers)

## Supported SIMD Types

- `f32x4` - 4x 32-bit floating point values
- `i32x4` - 4x 32-bit integer values  
- `u32x4` - 4x 32-bit unsigned integer values
- `f64x2` - 2x 64-bit floating point values

## Usage

### Basic Operations

```cpp
#include "engine/include/mercury.h"

// Create SIMD vectors
auto v1 = mercury::simd::make_f32x4(1.0f, 2.0f, 3.0f, 4.0f);
auto v2 = mercury::simd::make_f32x4(5.0f, 6.0f, 7.0f, 8.0f);

// Perform SIMD operations
auto result = mercury::simd::add_f32x4(v1, v2);
auto product = mercury::simd::mul_f32x4(v1, v2);

// Extract individual values
float x = mercury::simd::extract_lane_f32x4(result, 0);
float y = mercury::simd::extract_lane_f32x4(result, 1);
```

### Memory Operations

```cpp
// Load from memory
f32 data[4] = {1.1f, 2.2f, 3.3f, 4.4f};
auto loaded = mercury::simd::load_f32x4(data);

// Store to memory
f32 result[4];
mercury::simd::store_f32x4(result, loaded);
```

### Splat Operations

```cpp
// Create vector with all elements set to the same value
auto constant = mercury::simd::splat_f32x4(3.14f);
auto ones = mercury::simd::splat_i32x4(1);
```

## Emscripten WebAssembly SIMD

### Building with SIMD Support

The engine automatically enables WebAssembly SIMD when building with Emscripten by adding the `-msimd128` flag.

```bash
# Using the provided build script
./build-emsdk-simd.bat

# Or manually
mkdir build-emscripten
cd build-emscripten
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DEMSCRIPTEN=1 ..
cmake --build .
```

### Browser Compatibility

WebAssembly SIMD is supported by:
- Chrome 91+ (May 2021)
- Firefox 89+ (June 2021)
- Safari 16.4+ (March 2023)
- Node.js 16.4+ (June 2021)

### Detection

The code automatically detects SIMD support:

```cpp
#if defined(MERCURY_LL_OS_EMSCRIPTEN)
#ifdef __wasm_simd128__
    // WebAssembly SIMD is available
    printf("WebAssembly SIMD enabled\n");
#else
    // Fallback to scalar operations
    printf("WebAssembly SIMD not available\n");
#endif
#endif
```

## Performance Considerations

### WebAssembly SIMD Limitations

According to the [Emscripten SIMD documentation](https://emscripten.org/docs/porting/simd.html), some WebAssembly SIMD instructions may have performance implications:

1. **Shift operations** - Use constant shift amounts when possible
2. **8-bit operations** - Some i8x16 operations are emulated with multiple instructions
3. **NaN handling** - f32x4/f64x2 min/max operations may be slower due to NaN propagation semantics
4. **Type conversions** - Some truncation and conversion operations are emulated

### Optimization Tips

1. **Use constant values** for shift amounts and swizzle operations
2. **Prefer f32x4/f64x2 pmin/pmax** over min/max when possible
3. **Batch operations** to maximize SIMD utilization
4. **Profile performance** in target browsers

## Testing

Run the testbed to verify SIMD functionality:

```bash
# Native build
./build-windows.bat  # or build-linux.bat, build-macos.bat

# Emscripten build
./build-emsdk-simd.bat
```

The testbed will output platform detection and test results for all SIMD operations.

## Implementation Details

### Header Files

- `engine/include/mercury_api.h` - Platform detection and type definitions
- `engine/include/mercury_simd.h` - Cross-platform SIMD operations
- `engine/include/mercury.h` - Main engine header (includes SIMD)

### Compiler Flags

- **Emscripten**: `-msimd128` (automatically added)
- **ARM**: `-mfpu=neon` (typically enabled by default)
- **x86/x64**: `-msse2` or higher (typically enabled by default)

### Fallback Behavior

When SIMD is not available (e.g., older browsers), the engine falls back to scalar operations using struct-based implementations. This ensures compatibility while maintaining the same API.

## References

- [Emscripten SIMD Documentation](https://emscripten.org/docs/porting/simd.html)
- [WebAssembly SIMD Specification](https://github.com/WebAssembly/simd)
- [WebAssembly Roadmap](https://webassembly.org/roadmap/) 