#include "d3d12_utils.h"

#if defined(MERCURY_LL_GRAPHICS_D3D12)

using mercury::ll::graphics::Format;

static Format FallbackFormat()
{
    // No explicit Unknown in our enum, choose a safe default
    return Format::RGBA8_UNORM;
}

mercury::ll::graphics::Format FromDXGIFormat(DXGI_FORMAT dxgiFormat)
{
    switch (dxgiFormat)
    {
    case DXGI_FORMAT_R8_UNORM: return Format::R8_UNORM;
    case DXGI_FORMAT_R8_SNORM: return Format::R8_SNORM;
    case DXGI_FORMAT_R8_UINT:  return Format::R8_UINT;
    case DXGI_FORMAT_R8_SINT:  return Format::R8_SINT;

    case DXGI_FORMAT_R8G8_UNORM: return Format::RG8_UNORM;
    case DXGI_FORMAT_R8G8_SNORM: return Format::RG8_SNORM;
    case DXGI_FORMAT_R8G8_UINT:  return Format::RG8_UINT;
    case DXGI_FORMAT_R8G8_SINT:  return Format::RG8_SINT;

    case DXGI_FORMAT_R8G8B8A8_UNORM:       return Format::RGBA8_UNORM;
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:  return Format::RGBA8_UNORM_SRGB;
    case DXGI_FORMAT_R8G8B8A8_SNORM:       return Format::RGBA8_SNORM;
    case DXGI_FORMAT_R8G8B8A8_UINT:        return Format::RGBA8_UINT;
    case DXGI_FORMAT_R8G8B8A8_SINT:        return Format::RGBA8_SINT;

    case DXGI_FORMAT_B8G8R8A8_UNORM:       return Format::BGRA8_UNORM;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:  return Format::MTL_BGRA8_UNORM_SRGB; // closest sRGB variant in enum list

    case DXGI_FORMAT_R16_FLOAT:    return Format::R16_FLOAT;
    case DXGI_FORMAT_R16_UNORM:    return Format::R16_UNORM;
    case DXGI_FORMAT_R16_SNORM:    return Format::R16_SNORM;
    case DXGI_FORMAT_R16_UINT:     return Format::R16_UINT;
    case DXGI_FORMAT_R16_SINT:     return Format::R16_SINT;

    case DXGI_FORMAT_R16G16_FLOAT: return Format::RG16_FLOAT;
    case DXGI_FORMAT_R16G16_UNORM: return Format::RG16_UNORM;
    case DXGI_FORMAT_R16G16_SNORM: return Format::RG16_SNORM;
    case DXGI_FORMAT_R16G16_UINT:  return Format::RG16_UINT;
    case DXGI_FORMAT_R16G16_SINT:  return Format::RG16_SINT;

    case DXGI_FORMAT_R16G16B16A16_FLOAT: return Format::RGBA16_FLOAT;
    case DXGI_FORMAT_R16G16B16A16_UNORM: return Format::RGBA16_UNORM;
    case DXGI_FORMAT_R16G16B16A16_SNORM: return Format::RGBA16_SNORM;
    case DXGI_FORMAT_R16G16B16A16_UINT:  return Format::RGBA16_UINT;
    case DXGI_FORMAT_R16G16B16A16_SINT:  return Format::RGBA16_SINT;

    case DXGI_FORMAT_R32_FLOAT:    return Format::R32_FLOAT;
    case DXGI_FORMAT_R32_UINT:     return Format::R32_UINT;
    case DXGI_FORMAT_R32_SINT:     return Format::R32_SINT;

    case DXGI_FORMAT_R32G32_FLOAT: return Format::RG32_FLOAT;
    case DXGI_FORMAT_R32G32_UINT:  return Format::RG32_UINT;
    case DXGI_FORMAT_R32G32_SINT:  return Format::RG32_SINT;

    case DXGI_FORMAT_R32G32B32_FLOAT: return Format::RGB32_FLOAT;
    case DXGI_FORMAT_R32G32B32_UINT:  return Format::RGBA32_UINT; // no RGB32_UINT in enum; fallback to closest wider type
    case DXGI_FORMAT_R32G32B32_SINT:  return Format::RGBA32_SINT; // same reasoning

    case DXGI_FORMAT_R32G32B32A32_FLOAT: return Format::RGBA32_FLOAT;
    case DXGI_FORMAT_R32G32B32A32_UINT:  return Format::RGBA32_UINT;
    case DXGI_FORMAT_R32G32B32A32_SINT:  return Format::RGBA32_SINT;

    case DXGI_FORMAT_D16_UNORM:            return Format::DEPTH16_UNORM;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:    return Format::DEPTH24_UNORM_STENCIL8;
    case DXGI_FORMAT_D32_FLOAT:            return Format::DEPTH32_FLOAT;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return Format::DEPTH32_FLOAT_STENCIL8;

    case DXGI_FORMAT_B5G6R5_UNORM:  return Format::B5G6R5_UNORM;
    case DXGI_FORMAT_B5G5R5A1_UNORM:return Format::B5G5R5A1_UNORM;
    case DXGI_FORMAT_R10G10B10A2_UNORM: return Format::R10G10B10A2_UNORM;
    case DXGI_FORMAT_R10G10B10A2_UINT:  return Format::R10G10B10A2_UINT;
    case DXGI_FORMAT_R11G11B10_FLOAT:   return Format::RG11B10_FLOAT;
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:return Format::R9G9B9E5_UFLOAT;

    case DXGI_FORMAT_BC1_UNORM:       return Format::BC1_UNORM;
    case DXGI_FORMAT_BC1_UNORM_SRGB:  return Format::BC1_UNORM_SRGB;
    case DXGI_FORMAT_BC2_UNORM:       return Format::BC2_UNORM;
    case DXGI_FORMAT_BC2_UNORM_SRGB:  return Format::BC2_UNORM_SRGB;
    case DXGI_FORMAT_BC3_UNORM:       return Format::BC3_UNORM;
    case DXGI_FORMAT_BC3_UNORM_SRGB:  return Format::BC3_UNORM_SRGB;
    case DXGI_FORMAT_BC4_UNORM:       return Format::BC4_UNORM;
    case DXGI_FORMAT_BC4_SNORM:       return Format::BC4_SNORM;
    case DXGI_FORMAT_BC5_UNORM:       return Format::BC5_UNORM;
    case DXGI_FORMAT_BC5_SNORM:       return Format::BC5_SNORM;
    case DXGI_FORMAT_BC6H_UF16:       return Format::BC6H_UFLOAT;
    case DXGI_FORMAT_BC6H_SF16:       return Format::BC6H_SFLOAT;
    case DXGI_FORMAT_BC7_UNORM:       return Format::BC7_UNORM;
    case DXGI_FORMAT_BC7_UNORM_SRGB:  return Format::BC7_UNORM_SRGB;

    default: return FallbackFormat();
    }
}

DXGI_FORMAT ToDXGIFormat(mercury::ll::graphics::Format format)
{
    switch (format)
    {
    case Format::R8_UNORM: return DXGI_FORMAT_R8_UNORM;
    case Format::R8_SNORM: return DXGI_FORMAT_R8_SNORM;
    case Format::R8_UINT:  return DXGI_FORMAT_R8_UINT;
    case Format::R8_SINT:  return DXGI_FORMAT_R8_SINT;

    case Format::RG8_UNORM: return DXGI_FORMAT_R8G8_UNORM;
    case Format::RG8_SNORM: return DXGI_FORMAT_R8G8_SNORM;
    case Format::RG8_UINT:  return DXGI_FORMAT_R8G8_UINT;
    case Format::RG8_SINT:  return DXGI_FORMAT_R8G8_SINT;

    case Format::RGBA8_UNORM:       return DXGI_FORMAT_R8G8B8A8_UNORM;
    case Format::RGBA8_UNORM_SRGB:  return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case Format::RGBA8_SNORM:       return DXGI_FORMAT_R8G8B8A8_SNORM;
    case Format::RGBA8_UINT:        return DXGI_FORMAT_R8G8B8A8_UINT;
    case Format::RGBA8_SINT:        return DXGI_FORMAT_R8G8B8A8_SINT;

    case Format::BGRA8_UNORM:       return DXGI_FORMAT_B8G8R8A8_UNORM;

    case Format::R16_FLOAT: return DXGI_FORMAT_R16_FLOAT;
    case Format::R16_UNORM: return DXGI_FORMAT_R16_UNORM;
    case Format::R16_SNORM: return DXGI_FORMAT_R16_SNORM;
    case Format::R16_UINT:  return DXGI_FORMAT_R16_UINT;
    case Format::R16_SINT:  return DXGI_FORMAT_R16_SINT;

    case Format::RG16_FLOAT: return DXGI_FORMAT_R16G16_FLOAT;
    case Format::RG16_UNORM: return DXGI_FORMAT_R16G16_UNORM;
    case Format::RG16_SNORM: return DXGI_FORMAT_R16G16_SNORM;
    case Format::RG16_UINT:  return DXGI_FORMAT_R16G16_UINT;
    case Format::RG16_SINT:  return DXGI_FORMAT_R16G16_SINT;

    case Format::RGBA16_FLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case Format::RGBA16_UNORM: return DXGI_FORMAT_R16G16B16A16_UNORM;
    case Format::RGBA16_SNORM: return DXGI_FORMAT_R16G16B16A16_SNORM;
    case Format::RGBA16_UINT:  return DXGI_FORMAT_R16G16B16A16_UINT;
    case Format::RGBA16_SINT:  return DXGI_FORMAT_R16G16B16A16_SINT;

    case Format::R32_FLOAT: return DXGI_FORMAT_R32_FLOAT;
    case Format::R32_UINT:  return DXGI_FORMAT_R32_UINT;
    case Format::R32_SINT:  return DXGI_FORMAT_R32_SINT;

    case Format::RG32_FLOAT: return DXGI_FORMAT_R32G32_FLOAT;
    case Format::RG32_UINT:  return DXGI_FORMAT_R32G32_UINT;
    case Format::RG32_SINT:  return DXGI_FORMAT_R32G32_SINT;

    case Format::RGB32_FLOAT: return DXGI_FORMAT_R32G32B32_FLOAT;

    case Format::RGBA32_FLOAT: return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case Format::RGBA32_UINT:  return DXGI_FORMAT_R32G32B32A32_UINT;
    case Format::RGBA32_SINT:  return DXGI_FORMAT_R32G32B32A32_SINT;

    case Format::DEPTH16_UNORM:            return DXGI_FORMAT_D16_UNORM;
    case Format::DEPTH24_UNORM_STENCIL8:   return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case Format::DEPTH32_FLOAT:            return DXGI_FORMAT_D32_FLOAT;
    case Format::DEPTH32_FLOAT_STENCIL8:   return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    case Format::STENCIL8_UINT:            return DXGI_FORMAT_D24_UNORM_S8_UINT; // best-effort mapping

    case Format::B5G6R5_UNORM:  return DXGI_FORMAT_B5G6R5_UNORM;
    case Format::B5G5R5A1_UNORM:return DXGI_FORMAT_B5G5R5A1_UNORM;
    case Format::R10G10B10A2_UNORM: return DXGI_FORMAT_R10G10B10A2_UNORM;
    case Format::R10G10B10A2_UINT:  return DXGI_FORMAT_R10G10B10A2_UINT;
    case Format::RG11B10_FLOAT:    return DXGI_FORMAT_R11G11B10_FLOAT;
    case Format::R9G9B9E5_UFLOAT:  return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;

    case Format::BC1_UNORM:       return DXGI_FORMAT_BC1_UNORM;
    case Format::BC1_UNORM_SRGB:  return DXGI_FORMAT_BC1_UNORM_SRGB;
    case Format::BC2_UNORM:       return DXGI_FORMAT_BC2_UNORM;
    case Format::BC2_UNORM_SRGB:  return DXGI_FORMAT_BC2_UNORM_SRGB;
    case Format::BC3_UNORM:       return DXGI_FORMAT_BC3_UNORM;
    case Format::BC3_UNORM_SRGB:  return DXGI_FORMAT_BC3_UNORM_SRGB;
    case Format::BC4_UNORM:       return DXGI_FORMAT_BC4_UNORM;
    case Format::BC4_SNORM:       return DXGI_FORMAT_BC4_SNORM;
    case Format::BC5_UNORM:       return DXGI_FORMAT_BC5_UNORM;
    case Format::BC5_SNORM:       return DXGI_FORMAT_BC5_SNORM;
    case Format::BC6H_UFLOAT:     return DXGI_FORMAT_BC6H_UF16;
    case Format::BC6H_SFLOAT:     return DXGI_FORMAT_BC6H_SF16;
    case Format::BC7_UNORM:       return DXGI_FORMAT_BC7_UNORM;
    case Format::BC7_UNORM_SRGB:  return DXGI_FORMAT_BC7_UNORM_SRGB;

    default: return DXGI_FORMAT_UNKNOWN;
    }
}


#endif