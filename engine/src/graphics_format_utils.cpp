#include "ll/graphics.h"

using namespace mercury;
using namespace mercury::ll::graphics;


const FormatInfo& ll::graphics::GetFormatInfo(Format format)
{
    // We use a statically initialized table sized for the full u8 range.
    // Unknown entries default to zeros and non-compressed 1x1 blocks.
    static FormatInfo formatInfos[256];
    static bool initialized = false;

    if (!initialized)
    {
        // Default unknown format
        FormatInfo unknown = {};
        unknown.blockWidth = 1;
        unknown.blockHeight = 1;
        // Pre-fill all with unknown
        for (auto &fi : formatInfos)
            fi = unknown;

        auto makeUncompressed = [](u8 comps, u8 bytesPerPixel, bool srgb = false, bool depth = false) -> FormatInfo
        {
            FormatInfo fi = {};
            fi.numComponents = comps;
            fi.blockSize = bytesPerPixel;
            fi.blockWidth = 1;
            fi.blockHeight = 1;
            fi.isCompressed = false;
            fi.isSRGB = srgb;
            fi.isDepthFormat = depth;
            return fi;
        };
        auto makeCompressed = [](u8 comps, u8 bytesPerBlock, u8 blockW, u8 blockH, bool srgb = false) -> FormatInfo
        {
            FormatInfo fi = {};
            fi.numComponents = comps;
            fi.blockSize = bytesPerBlock;
            fi.blockWidth = blockW;
            fi.blockHeight = blockH;
            fi.isCompressed = true;
            fi.isSRGB = srgb;
            fi.isDepthFormat = false;
            return fi;
        };
        auto set = [&](Format fmt, const FormatInfo& info)
        {
            formatInfos[static_cast<u32>(fmt)] = info;
        };

        // Common uncompressed color formats
        set(Format::RGBA8_UNORM,           makeUncompressed(4, 4));
        set(Format::RGBA8_UNORM_SRGB,      makeUncompressed(4, 4, /*srgb*/true));
        set(Format::BGRA8_UNORM,           makeUncompressed(4, 4));
        set(Format::RGB32_FLOAT,           makeUncompressed(3, 12));
        set(Format::RGBA16_FLOAT,          makeUncompressed(4, 8));
        set(Format::RGBA32_FLOAT,          makeUncompressed(4, 16));
        set(Format::R8_UNORM,              makeUncompressed(1, 1));
        set(Format::RG8_UNORM,             makeUncompressed(2, 2));
        set(Format::R16_FLOAT,             makeUncompressed(1, 2));
        set(Format::RG16_FLOAT,            makeUncompressed(2, 4));
        set(Format::R32_FLOAT,             makeUncompressed(1, 4));
        set(Format::RG32_FLOAT,            makeUncompressed(2, 8));

        // Depth / Stencil formats (use common API block sizes)
        set(Format::DEPTH24_UNORM_STENCIL8,    makeUncompressed(2, 4, /*srgb*/false, /*depth*/true));
        set(Format::DEPTH32_FLOAT,             makeUncompressed(1, 4, /*srgb*/false, /*depth*/true));
        set(Format::DEPTH16_UNORM,             makeUncompressed(1, 2, /*srgb*/false, /*depth*/true));
        set(Format::STENCIL8_UINT,             makeUncompressed(1, 1, /*srgb*/false, /*depth*/true));
        set(Format::DEPTH32_FLOAT_STENCIL8,    makeUncompressed(2, 8, /*srgb*/false, /*depth*/true));
        set(Format::DEPTH16_UNORM_STENCIL8,    makeUncompressed(2, 4, /*srgb*/false, /*depth*/true)); // typically 32-bit block

        // Integer/normalized variants (sizes by component width)
        set(Format::R8_SNORM,              makeUncompressed(1, 1));
        set(Format::R8_UINT,               makeUncompressed(1, 1));
        set(Format::R8_SINT,               makeUncompressed(1, 1));
        set(Format::RG8_SNORM,             makeUncompressed(2, 2));
        set(Format::RG8_UINT,              makeUncompressed(2, 2));
        set(Format::RG8_SINT,              makeUncompressed(2, 2));
        set(Format::RGBA8_SNORM,           makeUncompressed(4, 4));
        set(Format::RGBA8_UINT,            makeUncompressed(4, 4));
        set(Format::RGBA8_SINT,            makeUncompressed(4, 4));

        set(Format::R16_UNORM,             makeUncompressed(1, 2));
        set(Format::R16_SNORM,             makeUncompressed(1, 2));
        set(Format::R16_UINT,              makeUncompressed(1, 2));
        set(Format::R16_SINT,              makeUncompressed(1, 2));
        set(Format::RG16_UNORM,            makeUncompressed(2, 4));
        set(Format::RG16_SNORM,            makeUncompressed(2, 4));
        set(Format::RG16_UINT,             makeUncompressed(2, 4));
        set(Format::RG16_SINT,             makeUncompressed(2, 4));
        set(Format::RGBA16_UNORM,          makeUncompressed(4, 8));
        set(Format::RGBA16_SNORM,          makeUncompressed(4, 8));
        set(Format::RGBA16_UINT,           makeUncompressed(4, 8));
        set(Format::RGBA16_SINT,           makeUncompressed(4, 8));

        set(Format::R32_UINT,              makeUncompressed(1, 4));
        set(Format::R32_SINT,              makeUncompressed(1, 4));
        set(Format::RG32_UINT,             makeUncompressed(2, 8));
        set(Format::RG32_SINT,             makeUncompressed(2, 8));
        set(Format::RGBA32_UINT,           makeUncompressed(4, 16));
        set(Format::RGBA32_SINT,           makeUncompressed(4, 16));

        set(Format::R64_FLOAT,             makeUncompressed(1, 8));
        set(Format::RG64_FLOAT,            makeUncompressed(2, 16));
        set(Format::RGBA64_FLOAT,          makeUncompressed(4, 32));

        set(Format::R10G10B10A2_UNORM,     makeUncompressed(4, 4));
        set(Format::R10G10B10A2_UINT,      makeUncompressed(4, 4));
        set(Format::RG11B10_FLOAT,         makeUncompressed(3, 4));
        set(Format::R9G9B9E5_UFLOAT,       makeUncompressed(3, 4));
        set(Format::B5G6R5_UNORM,          makeUncompressed(3, 2));
        set(Format::B5G5R5A1_UNORM,        makeUncompressed(4, 2));
        set(Format::R4G4_UNORM,            makeUncompressed(2, 1));
        set(Format::R4G4B4A4_UNORM,        makeUncompressed(4, 2));
        set(Format::B4G4R4A4_UNORM,        makeUncompressed(4, 2));
        set(Format::R5G5B5A1_UNORM,        makeUncompressed(4, 2));
        set(Format::A1R5G5B5_UNORM,        makeUncompressed(4, 2));

        // Block compressed (BCn)
        set(Format::BC1_UNORM,             makeCompressed(4, 8, 4, 4));
        set(Format::BC1_UNORM_SRGB,        makeCompressed(4, 8, 4, 4, /*srgb*/true));
        set(Format::BC2_UNORM,             makeCompressed(4, 16, 4, 4));
        set(Format::BC2_UNORM_SRGB,        makeCompressed(4, 16, 4, 4, /*srgb*/true));
        set(Format::BC3_UNORM,             makeCompressed(4, 16, 4, 4));
        set(Format::BC3_UNORM_SRGB,        makeCompressed(4, 16, 4, 4, /*srgb*/true));
        set(Format::BC4_UNORM,             makeCompressed(1, 8, 4, 4));
        set(Format::BC4_SNORM,             makeCompressed(1, 8, 4, 4));
        set(Format::BC5_UNORM,             makeCompressed(2, 16, 4, 4));
        set(Format::BC5_SNORM,             makeCompressed(2, 16, 4, 4));
        set(Format::BC6H_UFLOAT,           makeCompressed(3, 16, 4, 4));
        set(Format::BC6H_SFLOAT,           makeCompressed(3, 16, 4, 4));
        set(Format::BC7_UNORM,             makeCompressed(4, 16, 4, 4));
        set(Format::BC7_UNORM_SRGB,        makeCompressed(4, 16, 4, 4, /*srgb*/true));

        // ASTC
        set(Format::ASTC_4X4_UNORM,        makeCompressed(4, 16, 4, 4));
        set(Format::ASTC_4X4_UNORM_SRGB,   makeCompressed(4, 16, 4, 4, /*srgb*/true));

        // ETC2
        set(Format::ETC2_RGB_UNORM,        makeCompressed(3, 8, 4, 4));
        set(Format::ETC2_RGB_UNORM_SRGB,   makeCompressed(3, 8, 4, 4, /*srgb*/true));

        // Metal sRGB aliases
        set(Format::MTL_A8_UNORM,              makeUncompressed(1, 1));
        set(Format::MTL_R8_UNORM_SRGB,        makeUncompressed(1, 1, /*srgb*/true));
        set(Format::MTL_RG8_UNORM_SRGB,       makeUncompressed(2, 2, /*srgb*/true));
        set(Format::MTL_BGRA8_UNORM_SRGB,     makeUncompressed(4, 4, /*srgb*/true));
        set(Format::MTL_RGBA16_UNORM_SRGB,    makeUncompressed(4, 8, /*srgb*/true));
        set(Format::MTL_RGBA32_FLOAT_SRGB,    makeUncompressed(4, 16, /*srgb*/true));
        set(Format::MTL_DEPTH24_UNORM_STENCIL8, makeUncompressed(2, 4, /*srgb*/false, /*depth*/true));
        set(Format::MTL_DEPTH32_FLOAT_STENCIL8, makeUncompressed(2, 8, /*srgb*/false, /*depth*/true));
        set(Format::MTL_BGR10_XR_SRGB,        makeUncompressed(3, 4, /*srgb*/true));

        // WebGPU
        set(Format::WGPU_BGRA8_UNORM_SRGB,    makeUncompressed(4, 4, /*srgb*/true));
        set(Format::WGPU_DEPTH24PLUS,         makeUncompressed(1, 4, /*srgb*/false, /*depth*/true));
        set(Format::WGPU_DEPTH24PLUS_STENCIL8,makeUncompressed(2, 4, /*srgb*/false, /*depth*/true)); // treat as D24S8-like 32-bit

        initialized = true;
    }

    u32 index = static_cast<u32>(format);
    // Safe guard against out-of-range enum values
    return formatInfos[index];
}
