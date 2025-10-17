#include "mercury_embedded_shaders.h"

#ifdef MERCURY_LL_GRAPHICS_METAL

namespace mercury::ll::graphics::embedded_shaders {

mercury::ll::graphics::ShaderBytecodeView TestTriangleVS()
{
	static const char data[] = R"(#include <metal_stdlib>
#include <metal_math>
#include <metal_texture>
using namespace metal;

#line 29 "D:/Projects/mercury/engine/shaders/mercury_base.slang"
struct TestTriangleVS_Result_0
{
    float4 position_0 [[position]];
    float4 color_0 [[user(COLOR)]];
};


#line 29
struct BaseVertexOutputColorOnly_0
{
    float4 position_1;
    float4 color_1;
};


#line 29
[[vertex]] TestTriangleVS_Result_0 TestTriangleVS(uint vertexID_0 [[vertex_id]])
{

#line 10 "D:/Projects/mercury/engine/shaders/mercury.slang"
    array<float2, int(3)> positions_0 = { float2(0.0, 0.5), float2(0.5, -0.5), float2(-0.5, -0.5) };
    array<float4, int(3)> colors_0 = { float4(1.0, 0.0, 0.0, 1.0), float4(0.0, 1.0, 0.0, 1.0), float4(0.0, 0.0, 1.0, 1.0) };

#line 8
    thread BaseVertexOutputColorOnly_0 output_0;

#line 13
    (&output_0)->position_1 = float4(positions_0[vertexID_0], 0.0, 1.0);
    (&output_0)->color_1 = colors_0[vertexID_0];

#line 14
    thread TestTriangleVS_Result_0 _S1;

#line 14
    (&_S1)->position_0 = output_0.position_1;

#line 14
    (&_S1)->color_0 = output_0.color_1;

#line 14
    return _S1;
}

)";
	return { data, sizeof(data) };
}

mercury::ll::graphics::ShaderBytecodeView TestTrianglePS()
{
	static const char data[] = R"(#include <metal_stdlib>
#include <metal_math>
#include <metal_texture>
using namespace metal;

#line 3 "D:/Projects/mercury/engine/shaders/mercury_base.slang"
struct pixelOutput_0
{
    float4 output_0 [[color(0)]];
};


#line 3
struct pixelInput_0
{
    float4 color_0 [[user(COLOR)]];
};


#line 20 "D:/Projects/mercury/engine/shaders/mercury.slang"
[[fragment]] pixelOutput_0 TestTrianglePS(pixelInput_0 _S1 [[stage_in]], float4 position_0 [[position]])
{

#line 20
    pixelOutput_0 _S2 = { _S1.color_0 };

    return _S2;
}

)";
	return { data, sizeof(data) };
}

mercury::ll::graphics::ShaderBytecodeView DedicatedSpriteVS()
{
	static const char data[] = R"(#include <metal_stdlib>
#include <metal_math>
#include <metal_texture>
using namespace metal;

#line 8 "D:/Projects/mercury/engine/shaders/mercury_base.slang"
struct PackedColor_0
{
    uint packed_0;
};


#line 11
float4 PackedColor_toFloat4_0(const PackedColor_0 thread* this_0)
{
    thread float4 color_0;
    color_0.x = float(((this_0->packed_0) >> 24U) & 255U) / 255.0;
    color_0.y = float(((this_0->packed_0) >> 16U) & 255U) / 255.0;
    color_0.z = float(((this_0->packed_0) >> 8U) & 255U) / 255.0;
    color_0.w = float((this_0->packed_0) & 255U) / 255.0;
    return color_0;
}


struct BaseVertexOutput_0
{
    float4 position_0;
    float2 texcoord_0;
    float4 color_1;
};


#line 35
struct DedicatedSpriteParameters_0
{
    float2 position_1;
    float2 size_0;
    float4 uvs_0;
    float angle_0;
    PackedColor_0 colorPacked_0;
};


#line 35
struct EntryPointParams_0
{
    DedicatedSpriteParameters_0 params_0;
};


#line 3
struct MercuryScene_0
{
    float4 canvasSize_0;
};


#line 26 "D:/Projects/mercury/engine/shaders/mercury.slang"
struct KernelContext_0
{
    EntryPointParams_0 constant* entryPointParams_0;
    MercuryScene_0 constant* entryPointParams_scene_0;
};


#line 26
BaseVertexOutput_0 DedicatedSpriteVS_0(const uint thread* vertexID_0, KernelContext_0 thread* kernelContext_0)
{


    float2 _S1 = float2(1.0, 1.0);

#line 30
    array<float2, int(4)> positions_ndc_0 = { float2(-1.0, 1.0), _S1, float2(-1.0, -1.0), float2(1.0, -1.0) };
    array<float2, int(4)> uvs_1 = { float2(0.0, 0.0), float2(1.0, 0.0), float2(0.0, 1.0), _S1 };

#line 28
    thread BaseVertexOutput_0 output_0;

#line 34
    (&output_0)->position_0 = float4(positions_ndc_0[*vertexID_0] * kernelContext_0->entryPointParams_scene_0->canvasSize_0.zw, 0.0, 1.0);
    (&output_0)->texcoord_0 = uvs_1[*vertexID_0];

#line 35
    thread PackedColor_0 _S2 = kernelContext_0->entryPointParams_0->params_0.colorPacked_0;

#line 35
    float4 _S3 = PackedColor_toFloat4_0(&_S2);
    (&output_0)->color_1 = _S3;

    return output_0;
}


#line 38
struct DedicatedSpriteVS_Result_0
{
    float4 position_2 [[position]];
    float2 texcoord_1 [[user(TEXCOORD)]];
    float4 color_2 [[user(COLOR)]];
};


#line 38
[[vertex]] DedicatedSpriteVS_Result_0 DedicatedSpriteVS(uint vertexID_1 [[vertex_id]])
{

#line 38
    thread uint _S4 = vertexID_1;

#line 38
    thread KernelContext_0 kernelContext_1;

#line 38
    BaseVertexOutput_0 _S5 = DedicatedSpriteVS_0(&_S4, &kernelContext_1);

#line 38
    thread DedicatedSpriteVS_Result_0 _S6;

#line 38
    (&_S6)->position_2 = _S5.position_0;

#line 38
    (&_S6)->texcoord_1 = _S5.texcoord_0;

#line 38
    (&_S6)->color_2 = _S5.color_1;

#line 38
    return _S6;
}

)";
	return { data, sizeof(data) };
}

mercury::ll::graphics::ShaderBytecodeView DedicatedSpriteColorPS()
{
	static const char data[] = R"(#include <metal_stdlib>
#include <metal_math>
#include <metal_texture>
using namespace metal;

#line 3 "D:/Projects/mercury/engine/shaders/mercury_base.slang"
struct pixelOutput_0
{
    float4 output_0 [[color(0)]];
};


#line 22
struct pixelInput_0
{
    float2 texcoord_0 [[user(TEXCOORD)]];
    float4 color_0 [[user(COLOR)]];
};


#line 42 "D:/Projects/mercury/engine/shaders/mercury.slang"
[[fragment]] pixelOutput_0 DedicatedSpriteColorPS(pixelInput_0 _S1 [[stage_in]], float4 position_0 [[position]])
{

#line 42
    pixelOutput_0 _S2 = { _S1.color_0 };

    return _S2;
}

)";
	return { data, sizeof(data) };
}

mercury::ll::graphics::ShaderBytecodeView DedicatedSpritePS()
{
	static const char data[] = R"(#include <metal_stdlib>
#include <metal_math>
#include <metal_texture>
using namespace metal;

#line 3 "D:/Projects/mercury/engine/shaders/mercury_base.slang"
struct pixelOutput_0
{
    float4 output_0 [[color(0)]];
};


#line 10 "D:/Projects/mercury/engine/shaders/mercury.slang"
struct pixelInput_0
{
    float2 texcoord_0 [[user(TEXCOORD)]];
    float4 color_0 [[user(COLOR)]];
};


#line 48
[[fragment]] pixelOutput_0 DedicatedSpritePS(pixelInput_0 _S1 [[stage_in]], float4 position_0 [[position]])
{

#line 48
    pixelOutput_0 _S2 = { _S1.color_0 * float4(_S1.texcoord_0, 0.0, 1.0) };

    return _S2;
}

)";
	return { data, sizeof(data) };
}

} // namespace mercury::ll::graphics::embedded_shaders
#endif