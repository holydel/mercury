#include "metal_graphics.h"
#include "mercury_log.h"
#include "mercury_utils.h"

#ifdef MERCURY_LL_GRAPHICS_METAL

using namespace mercury::ll::graphics;

// Metal format conversion utilities
MTLPixelFormat ConvertFormatToMetal(Format format) {
    switch (format) {
        case Format::RGBA8_UNORM:        return MTLPixelFormatRGBA8Unorm;
        case Format::RGBA8_UNORM_SRGB:   return MTLPixelFormatRGBA8Unorm_sRGB;
        case Format::BGRA8_UNORM:        return MTLPixelFormatBGRA8Unorm;
        case Format::RGBA16_FLOAT:       return MTLPixelFormatRGBA16Float;
        case Format::RGBA32_FLOAT:       return MTLPixelFormatRGBA32Float;
        case Format::R8_UNORM:           return MTLPixelFormatR8Unorm;
        case Format::RG8_UNORM:          return MTLPixelFormatRG8Unorm;
        case Format::R16_FLOAT:          return MTLPixelFormatR16Float;
        case Format::RG16_FLOAT:         return MTLPixelFormatRG16Float;
        case Format::R32_FLOAT:          return MTLPixelFormatR32Float;
        case Format::RG32_FLOAT:         return MTLPixelFormatRG32Float;
        case Format::DEPTH24_UNORM_STENCIL8: return MTLPixelFormatDepth24Unorm_Stencil8;
        case Format::DEPTH32_FLOAT:      return MTLPixelFormatDepth32Float;
        case Format::DEPTH16_UNORM:      return MTLPixelFormatDepth16Unorm;
        case Format::R8_SNORM:           return MTLPixelFormatR8Snorm;
        case Format::R8_UINT:            return MTLPixelFormatR8Uint;
        case Format::R8_SINT:            return MTLPixelFormatR8Sint;
        case Format::RG8_SNORM:          return MTLPixelFormatRG8Snorm;
        case Format::RG8_UINT:           return MTLPixelFormatRG8Uint;
        case Format::RG8_SINT:           return MTLPixelFormatRG8Sint;
        case Format::RGBA8_SNORM:        return MTLPixelFormatRGBA8Snorm;
        case Format::RGBA8_UINT:         return MTLPixelFormatRGBA8Uint;
        case Format::RGBA8_SINT:         return MTLPixelFormatRGBA8Sint;
        case Format::R16_UNORM:          return MTLPixelFormatR16Unorm;
        case Format::R16_SNORM:          return MTLPixelFormatR16Snorm;
        case Format::R16_UINT:           return MTLPixelFormatR16Uint;
        case Format::R16_SINT:           return MTLPixelFormatR16Sint;
        case Format::RG16_UNORM:         return MTLPixelFormatRG16Unorm;
        case Format::RG16_SNORM:         return MTLPixelFormatRG16Snorm;
        case Format::RG16_UINT:          return MTLPixelFormatRG16Uint;
        case Format::RG16_SINT:          return MTLPixelFormatRG16Sint;
        case Format::RGBA16_UNORM:       return MTLPixelFormatRGBA16Unorm;
        case Format::RGBA16_SNORM:       return MTLPixelFormatRGBA16Snorm;
        case Format::RGBA16_UINT:        return MTLPixelFormatRGBA16Uint;
        case Format::RGBA16_SINT:        return MTLPixelFormatRGBA16Sint;
        case Format::R32_UINT:           return MTLPixelFormatR32Uint;
        case Format::R32_SINT:           return MTLPixelFormatR32Sint;
        case Format::RG32_UINT:          return MTLPixelFormatRG32Uint;
        case Format::RG32_SINT:          return MTLPixelFormatRG32Sint;
        case Format::RGBA32_UINT:        return MTLPixelFormatRGBA32Uint;
        case Format::RGBA32_SINT:        return MTLPixelFormatRGBA32Sint;
        case Format::R64_FLOAT:          return MTLPixelFormatR32Float; // Fallback to R32Float
        case Format::RG64_FLOAT:         return MTLPixelFormatRG32Float; // Fallback to RG32Float
        case Format::RGBA64_FLOAT:       return MTLPixelFormatRGBA32Float; // Fallback to RGBA32Float
        case Format::R10G10B10A2_UNORM:  return MTLPixelFormatRGB10A2Unorm;
        case Format::R10G10B10A2_UINT:   return MTLPixelFormatRGB10A2Uint;
        case Format::RG11B10_FLOAT:      return MTLPixelFormatRG11B10Float;
        case Format::R9G9B9E5_UFLOAT:    return MTLPixelFormatRGB9E5Float;
        case Format::B5G6R5_UNORM:       return MTLPixelFormatB5G6R5Unorm;
        case Format::B5G5R5A1_UNORM:     return MTLPixelFormatBGR5A1Unorm;
        case Format::A1R5G5B5_UNORM:     return MTLPixelFormatABGR4Unorm;
        case Format::STENCIL8_UINT:      return MTLPixelFormatStencil8;
        case Format::DEPTH32_FLOAT_STENCIL8: return MTLPixelFormatDepth32Float_Stencil8;
        case Format::DEPTH16_UNORM_STENCIL8: return MTLPixelFormatDepth24Unorm_Stencil8; // Fallback to Depth24Unorm_Stencil8
        default:
            MLOG_WARNING(u8"Unsupported format conversion to Metal: %d", static_cast<int>(format));
            return MTLPixelFormatRGBA8Unorm;
    }
}

Format ConvertFormatFromMetal(MTLPixelFormat format) {
    switch (format) {
        case MTLPixelFormatRGBA8Unorm:        return Format::RGBA8_UNORM;
        case MTLPixelFormatRGBA8Unorm_sRGB:   return Format::RGBA8_UNORM_SRGB;
        case MTLPixelFormatBGRA8Unorm:        return Format::BGRA8_UNORM;
        case MTLPixelFormatRGBA16Float:       return Format::RGBA16_FLOAT;
        case MTLPixelFormatRGBA32Float:       return Format::RGBA32_FLOAT;
        case MTLPixelFormatR8Unorm:           return Format::R8_UNORM;
        case MTLPixelFormatRG8Unorm:          return Format::RG8_UNORM;
        case MTLPixelFormatR16Float:          return Format::R16_FLOAT;
        case MTLPixelFormatRG16Float:         return Format::RG16_FLOAT;
        case MTLPixelFormatR32Float:          return Format::R32_FLOAT;
        case MTLPixelFormatRG32Float:         return Format::RG32_FLOAT;
        case MTLPixelFormatDepth24Unorm_Stencil8: return Format::DEPTH24_UNORM_STENCIL8;
        case MTLPixelFormatDepth32Float:      return Format::DEPTH32_FLOAT;
        case MTLPixelFormatDepth16Unorm:      return Format::DEPTH16_UNORM;
        case MTLPixelFormatR8Snorm:           return Format::R8_SNORM;
        case MTLPixelFormatR8Uint:            return Format::R8_UINT;
        case MTLPixelFormatR8Sint:            return Format::R8_SINT;
        case MTLPixelFormatRG8Snorm:          return Format::RG8_SNORM;
        case MTLPixelFormatRG8Uint:           return Format::RG8_UINT;
        case MTLPixelFormatRG8Sint:           return Format::RG8_SINT;
        case MTLPixelFormatRGBA8Snorm:        return Format::RGBA8_SNORM;
        case MTLPixelFormatRGBA8Uint:         return Format::RGBA8_UINT;
        case MTLPixelFormatRGBA8Sint:         return Format::RGBA8_SINT;
        case MTLPixelFormatR16Unorm:          return Format::R16_UNORM;
        case MTLPixelFormatR16Snorm:          return Format::R16_SNORM;
        case MTLPixelFormatR16Uint:           return Format::R16_UINT;
        case MTLPixelFormatR16Sint:           return Format::R16_SINT;
        case MTLPixelFormatRG16Unorm:         return Format::RG16_UNORM;
        case MTLPixelFormatRG16Snorm:         return Format::RG16_SNORM;
        case MTLPixelFormatRG16Uint:          return Format::RG16_UINT;
        case MTLPixelFormatRG16Sint:          return Format::RG16_SINT;
        case MTLPixelFormatRGBA16Unorm:       return Format::RGBA16_UNORM;
        case MTLPixelFormatRGBA16Snorm:       return Format::RGBA16_SNORM;
        case MTLPixelFormatRGBA16Uint:        return Format::RGBA16_UINT;
        case MTLPixelFormatRGBA16Sint:        return Format::RGBA16_SINT;
        case MTLPixelFormatR32Uint:           return Format::R32_UINT;
        case MTLPixelFormatR32Sint:           return Format::R32_SINT;
        case MTLPixelFormatRG32Uint:          return Format::RG32_UINT;
        case MTLPixelFormatRG32Sint:          return Format::RG32_SINT;
        case MTLPixelFormatRGBA32Uint:        return Format::RGBA32_UINT;
        case MTLPixelFormatRGBA32Sint:        return Format::RGBA32_SINT;
        // R64_FLOAT, RG64_FLOAT, RGBA64_FLOAT fallbacks handled by existing R32Float cases above
        case MTLPixelFormatRGB10A2Unorm:      return Format::R10G10B10A2_UNORM;
        case MTLPixelFormatRGB10A2Uint:       return Format::R10G10B10A2_UINT;
        case MTLPixelFormatRG11B10Float:      return Format::RG11B10_FLOAT;
        case MTLPixelFormatRGB9E5Float:       return Format::R9G9B9E5_UFLOAT;
        case MTLPixelFormatB5G6R5Unorm:       return Format::B5G6R5_UNORM;
        case MTLPixelFormatBGR5A1Unorm:       return Format::B5G5R5A1_UNORM;
        case MTLPixelFormatABGR4Unorm:        return Format::A1R5G5B5_UNORM;
        case MTLPixelFormatStencil8:          return Format::STENCIL8_UINT;
        case MTLPixelFormatDepth32Float_Stencil8: return Format::DEPTH32_FLOAT_STENCIL8;
        // MTLPixelFormatDepth16Unorm_Stencil8 doesn't exist, using Depth24Unorm_Stencil8
        default:
            MLOG_WARNING(u8"Unsupported format conversion from Metal: %d", static_cast<int>(format));
            return Format::RGBA8_UNORM;
    }
}

// Metal debug utilities
void SetMetalDebugName(id<MTLResource> resource, const char* name) {
    if (resource && name) {
        [resource setLabel:[NSString stringWithUTF8String:name]];
    }
}

// Metal error checking
void CheckMetalError(NSError* error, const char* operation) {
    if (error) {
        MLOG_ERROR(u8"Metal %s failed: %s", operation, 
                  reinterpret_cast<const char8_t*>([error.localizedDescription UTF8String]));
    }
}

// Metal memory management utilities
mercury::u64 GetMetalDeviceMemorySize(id<MTLDevice> device) {
    if (@available(macOS 10.15, iOS 13.0, *)) {
        return device.recommendedMaxWorkingSetSize;
    }
    return 0; // Fallback for older versions
}

mercury::u32 GetMetalVendorId(id<MTLDevice> device) {
    // Metal doesn't expose vendor ID directly, but we can infer from device name
    NSString* deviceName = device.name;
    if ([deviceName containsString:@"Apple"]) {
        return 0x106B; // Apple vendor ID
    }
    return 0; // Unknown vendor
}

mercury::u32 GetMetalDeviceId(id<MTLDevice> device) {
    // Metal doesn't expose device ID directly
    return 0; // Not available in Metal API
}

const char* GetMetalDeviceName(id<MTLDevice> device) {
    return [device.name UTF8String];
}

#endif // MERCURY_LL_GRAPHICS_METAL
