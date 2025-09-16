#include "webgpu_utils.h"

#if defined(MERCURY_LL_GRAPHICS_WEBGPU)
namespace wgpu_utils
{
    // Helper functions to convert WebGPU types to human-readable strings
    const char *GetBackendTypeString(wgpu::BackendType backendType)
    {
        switch (backendType)
        {
        case wgpu::BackendType::Undefined:
            return "Undefined";
        case wgpu::BackendType::Null:
            return "Null";
        case wgpu::BackendType::WebGPU:
            return "WebGPU";
        case wgpu::BackendType::D3D11:
            return "DirectX 11";
        case wgpu::BackendType::D3D12:
            return "DirectX 12";
        case wgpu::BackendType::Metal:
            return "Metal";
        case wgpu::BackendType::Vulkan:
            return "Vulkan";
        case wgpu::BackendType::OpenGL:
            return "OpenGL";
        case wgpu::BackendType::OpenGLES:
            return "OpenGL ES";
        default:
            return "Unknown";
        }
    }

    const char *GetAdapterTypeString(wgpu::AdapterType adapterType)
    {
        switch (adapterType)
        {
        case wgpu::AdapterType::DiscreteGPU:
            return "Discrete GPU";
        case wgpu::AdapterType::IntegratedGPU:
            return "Integrated GPU";
        case wgpu::AdapterType::CPU:
            return "CPU";
        case wgpu::AdapterType::Unknown:
            return "Unknown";
        default:
            return "Unknown";
        }
    }

    const char *GetTextureFormatString(wgpu::TextureFormat format)
    {
        switch (format)
        {
        case wgpu::TextureFormat::R8Unorm:
            return "R8Unorm";
        case wgpu::TextureFormat::R8Snorm:
            return "R8Snorm";
        case wgpu::TextureFormat::R8Uint:
            return "R8Uint";
        case wgpu::TextureFormat::R8Sint:
            return "R8Sint";
        case wgpu::TextureFormat::R16Uint:
            return "R16Uint";
        case wgpu::TextureFormat::R16Sint:
            return "R16Sint";
        case wgpu::TextureFormat::R16Float:
            return "R16Float";
        case wgpu::TextureFormat::RG8Unorm:
            return "RG8Unorm";
        case wgpu::TextureFormat::RG8Snorm:
            return "RG8Snorm";
        case wgpu::TextureFormat::RG8Uint:
            return "RG8Uint";
        case wgpu::TextureFormat::RG8Sint:
            return "RG8Sint";
        case wgpu::TextureFormat::R32Float:
            return "R32Float";
        case wgpu::TextureFormat::R32Uint:
            return "R32Uint";
        case wgpu::TextureFormat::R32Sint:
            return "R32Sint";
        case wgpu::TextureFormat::RG16Uint:
            return "RG16Uint";
        case wgpu::TextureFormat::RG16Sint:
            return "RG16Sint";
        case wgpu::TextureFormat::RG16Float:
            return "RG16Float";
        case wgpu::TextureFormat::RGBA8Unorm:
            return "RGBA8Unorm";
        case wgpu::TextureFormat::RGBA8UnormSrgb:
            return "RGBA8UnormSrgb";
        case wgpu::TextureFormat::RGBA8Snorm:
            return "RGBA8Snorm";
        case wgpu::TextureFormat::RGBA8Uint:
            return "RGBA8Uint";
        case wgpu::TextureFormat::RGBA8Sint:
            return "RGBA8Sint";
        case wgpu::TextureFormat::BGRA8Unorm:
            return "BGRA8Unorm";
        case wgpu::TextureFormat::BGRA8UnormSrgb:
            return "BGRA8UnormSrgb";
        case wgpu::TextureFormat::RGB10A2Unorm:
            return "RGB10A2Unorm";
        case wgpu::TextureFormat::RG11B10Ufloat:
            return "RG11B10Ufloat";
        case wgpu::TextureFormat::RGB9E5Ufloat:
            return "RGB9E5Ufloat";
        case wgpu::TextureFormat::RG32Float:
            return "RG32Float";
        case wgpu::TextureFormat::RG32Uint:
            return "RG32Uint";
        case wgpu::TextureFormat::RG32Sint:
            return "RG32Sint";
        case wgpu::TextureFormat::RGBA16Uint:
            return "RGBA16Uint";
        case wgpu::TextureFormat::RGBA16Sint:
            return "RGBA16Sint";
        case wgpu::TextureFormat::RGBA16Float:
            return "RGBA16Float";
        case wgpu::TextureFormat::RGBA32Float:
            return "RGBA32Float";
        case wgpu::TextureFormat::RGBA32Uint:
            return "RGBA32Uint";
        case wgpu::TextureFormat::RGBA32Sint:
            return "RGBA32Sint";
        case wgpu::TextureFormat::Stencil8:
            return "Stencil8";
        case wgpu::TextureFormat::Depth16Unorm:
            return "Depth16Unorm";
        case wgpu::TextureFormat::Depth24Plus:
            return "Depth24Plus";
        case wgpu::TextureFormat::Depth24PlusStencil8:
            return "Depth24PlusStencil8";
        case wgpu::TextureFormat::Depth32Float:
            return "Depth32Float";
        case wgpu::TextureFormat::Depth32FloatStencil8:
            return "Depth32FloatStencil8";
        case wgpu::TextureFormat::BC1RGBAUnorm:
            return "BC1RGBAUnorm";
        case wgpu::TextureFormat::BC1RGBAUnormSrgb:
            return "BC1RGBAUnormSrgb";
        case wgpu::TextureFormat::BC2RGBAUnorm:
            return "BC2RGBAUnorm";
        case wgpu::TextureFormat::BC2RGBAUnormSrgb:
            return "BC2RGBAUnormSrgb";
        case wgpu::TextureFormat::BC3RGBAUnorm:
            return "BC3RGBAUnorm";
        case wgpu::TextureFormat::BC3RGBAUnormSrgb:
            return "BC3RGBAUnormSrgb";
        case wgpu::TextureFormat::BC4RUnorm:
            return "BC4RUnorm";
        case wgpu::TextureFormat::BC4RSnorm:
            return "BC4RSnorm";
        case wgpu::TextureFormat::BC5RGUnorm:
            return "BC5RGUnorm";
        case wgpu::TextureFormat::BC5RGSnorm:
            return "BC5RGSnorm";
        case wgpu::TextureFormat::BC6HRGBUfloat:
            return "BC6HRGBUfloat";
        case wgpu::TextureFormat::BC6HRGBFloat:
            return "BC6HRGBFloat";
        case wgpu::TextureFormat::BC7RGBAUnorm:
            return "BC7RGBAUnorm";
        case wgpu::TextureFormat::BC7RGBAUnormSrgb:
            return "BC7RGBAUnormSrgb";
        case wgpu::TextureFormat::ETC2RGB8Unorm:
            return "ETC2RGB8Unorm";
        case wgpu::TextureFormat::ETC2RGB8UnormSrgb:
            return "ETC2RGB8UnormSrgb";
        case wgpu::TextureFormat::ETC2RGB8A1Unorm:
            return "ETC2RGB8A1Unorm";
        case wgpu::TextureFormat::ETC2RGB8A1UnormSrgb:
            return "ETC2RGB8A1UnormSrgb";
        case wgpu::TextureFormat::ETC2RGBA8Unorm:
            return "ETC2RGBA8Unorm";
        case wgpu::TextureFormat::ETC2RGBA8UnormSrgb:
            return "ETC2RGBA8UnormSrgb";
        case wgpu::TextureFormat::EACR11Unorm:
            return "EACR11Unorm";
        case wgpu::TextureFormat::EACR11Snorm:
            return "EACR11Snorm";
        case wgpu::TextureFormat::EACRG11Unorm:
            return "EACRG11Unorm";
        case wgpu::TextureFormat::EACRG11Snorm:
            return "EACRG11Snorm";
        case wgpu::TextureFormat::ASTC4x4Unorm:
            return "ASTC4x4Unorm";
        case wgpu::TextureFormat::ASTC4x4UnormSrgb:
            return "ASTC4x4UnormSrgb";
        case wgpu::TextureFormat::ASTC5x4Unorm:
            return "ASTC5x4Unorm";
        case wgpu::TextureFormat::ASTC5x4UnormSrgb:
            return "ASTC5x4UnormSrgb";
        case wgpu::TextureFormat::ASTC5x5Unorm:
            return "ASTC5x5Unorm";
        case wgpu::TextureFormat::ASTC5x5UnormSrgb:
            return "ASTC5x5UnormSrgb";
        case wgpu::TextureFormat::ASTC6x5Unorm:
            return "ASTC6x5Unorm";
        case wgpu::TextureFormat::ASTC6x5UnormSrgb:
            return "ASTC6x5UnormSrgb";
        case wgpu::TextureFormat::ASTC6x6Unorm:
            return "ASTC6x6Unorm";
        case wgpu::TextureFormat::ASTC6x6UnormSrgb:
            return "ASTC6x6UnormSrgb";
        case wgpu::TextureFormat::ASTC8x5Unorm:
            return "ASTC8x5Unorm";
        case wgpu::TextureFormat::ASTC8x5UnormSrgb:
            return "ASTC8x5UnormSrgb";
        case wgpu::TextureFormat::ASTC8x6Unorm:
            return "ASTC8x6Unorm";
        case wgpu::TextureFormat::ASTC8x6UnormSrgb:
            return "ASTC8x6UnormSrgb";
        case wgpu::TextureFormat::ASTC8x8Unorm:
            return "ASTC8x8Unorm";
        case wgpu::TextureFormat::ASTC8x8UnormSrgb:
            return "ASTC8x8UnormSrgb";
        case wgpu::TextureFormat::ASTC10x5Unorm:
            return "ASTC10x5Unorm";
        case wgpu::TextureFormat::ASTC10x5UnormSrgb:
            return "ASTC10x5UnormSrgb";
        case wgpu::TextureFormat::ASTC10x6Unorm:
            return "ASTC10x6Unorm";
        case wgpu::TextureFormat::ASTC10x6UnormSrgb:
            return "ASTC10x6UnormSrgb";
        case wgpu::TextureFormat::ASTC10x8Unorm:
            return "ASTC10x8Unorm";
        case wgpu::TextureFormat::ASTC10x8UnormSrgb:
            return "ASTC10x8UnormSrgb";
        case wgpu::TextureFormat::ASTC10x10Unorm:
            return "ASTC10x10Unorm";
        case wgpu::TextureFormat::ASTC10x10UnormSrgb:
            return "ASTC10x10UnormSrgb";
        case wgpu::TextureFormat::ASTC12x10Unorm:
            return "ASTC12x10Unorm";
        case wgpu::TextureFormat::ASTC12x10UnormSrgb:
            return "ASTC12x10UnormSrgb";
        case wgpu::TextureFormat::ASTC12x12Unorm:
            return "ASTC12x12Unorm";
        case wgpu::TextureFormat::ASTC12x12UnormSrgb:
            return "ASTC12x12UnormSrgb";
        default:
            return "Unknown";
        }
    }

    const char *GetPresentModeString(wgpu::PresentMode presentMode)
    {
        switch (presentMode)
        {
        case wgpu::PresentMode::Fifo:
            return "Fifo (VSync)";
        case wgpu::PresentMode::FifoRelaxed:
            return "FifoRelaxed (Adaptive VSync)";
        case wgpu::PresentMode::Immediate:
            return "Immediate (No VSync)";
        case wgpu::PresentMode::Mailbox:
            return "Mailbox (Triple Buffering)";
        default:
            return "Unknown";
        }
    }

    const char *GetCompositeAlphaModeString(wgpu::CompositeAlphaMode alphaMode)
    {
        switch (alphaMode)
        {
        case wgpu::CompositeAlphaMode::Auto:
            return "Auto";
        case wgpu::CompositeAlphaMode::Opaque:
            return "Opaque";
        case wgpu::CompositeAlphaMode::Premultiplied:
            return "Premultiplied";
        case wgpu::CompositeAlphaMode::Unpremultiplied:
            return "Unpremultiplied";
        case wgpu::CompositeAlphaMode::Inherit:
            return "Inherit";
        default:
            return "Unknown";
        }
    }
}
#endif