#include "mercury_embedded_shaders.h"

#ifdef MERCURY_LL_GRAPHICS_METAL

namespace mercury::ll::graphics::embedded_shaders {

mercury::ll::graphics::ShaderBytecodeView DedicatedSpriteVS()
{
	static const char data[] = R"(#include <metal_stdlib>
#include <metal_math>
#include <metal_texture>
using namespace metal;

#line 12 "D:/Projects/mercury/engine/shaders/mercury_base.slang"
struct PackedColor_0
{
    uint packed_0;
};


#line 12
PackedColor_0 PackedColor_x24init_0(uint packed_1)
{

#line 12
    thread PackedColor_0 _S1;

    (&_S1)->packed_0 = packed_1;

#line 12
    return _S1;
}

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


#line 39
struct DedicatedSpriteParameters_0
{
    float2 position_1;
    float2 size_0;
    float4 uvs_0;
    float angle_0;
    uint colorPacked_0;
};


#line 39
struct EntryPointParams_0
{
    DedicatedSpriteParameters_0 perObject_0;
};


#line 3
struct MercuryScene_0
{
    float4 prerptationMatrix_0;
    float4 canvasSize_0;
    float time_0;
    float deltaTime_0;
};


#line 6 "D:/Projects/mercury/engine/shaders/dedicated_sprite.slang"
struct KernelContext_0
{
    EntryPointParams_0 constant* entryPointParams_0;
    MercuryScene_0 constant* entryPointParams_perFrame_0;
};


#line 6
BaseVertexOutput_0 DedicatedSpriteVS_0(const uint thread* vertexID_0, KernelContext_0 thread* kernelContext_0)
{


    float2 _S2 = float2(-1.0, 1.0);

#line 10
    float2 _S3 = float2(1.0, 1.0);

#line 10
    array<float2, int(4)> positions_ndc_0 = { _S2, _S3, float2(-1.0, -1.0), float2(1.0, -1.0) };
    array<float2, int(4)> uvs_1 = { float2(0.0, 0.0), float2(1.0, 0.0), float2(0.0, 1.0), _S3 };



    float cosAngle_0 = cos(kernelContext_0->entryPointParams_0->perObject_0.angle_0);
    float sinAngle_0 = sin(kernelContext_0->entryPointParams_0->perObject_0.angle_0);

#line 8
    thread BaseVertexOutput_0 output_0;

#line 23
    (&output_0)->position_0 = float4(((((positions_ndc_0[*vertexID_0] * kernelContext_0->entryPointParams_0->perObject_0.size_0) * (matrix<float,int(2),int(2)> (cosAngle_0, - sinAngle_0, sinAngle_0, cosAngle_0)))) + kernelContext_0->entryPointParams_0->perObject_0.position_1) * kernelContext_0->entryPointParams_perFrame_0->canvasSize_0.zw + _S2, 0.0, 1.0);


    (&output_0)->texcoord_0 = uvs_1[*vertexID_0];

#line 26
    thread PackedColor_0 _S4 = PackedColor_x24init_0(kernelContext_0->entryPointParams_0->perObject_0.colorPacked_0);

#line 26
    float4 _S5 = PackedColor_toFloat4_0(&_S4);
    (&output_0)->color_1 = _S5;

    return output_0;
}


#line 29
struct DedicatedSpriteVS_Result_0
{
    float4 position_2 [[position]];
    float2 texcoord_1 [[user(TEXCOORD)]];
    float4 color_2 [[user(COLOR)]];
};


#line 29
[[vertex]] DedicatedSpriteVS_Result_0 DedicatedSpriteVS(uint vertexID_1 [[vertex_id]])
{

#line 29
    thread uint _S6 = vertexID_1;

#line 29
    thread KernelContext_0 kernelContext_1;

#line 29
    BaseVertexOutput_0 _S7 = DedicatedSpriteVS_0(&_S6, &kernelContext_1);

#line 29
    thread DedicatedSpriteVS_Result_0 _S8;

#line 29
    (&_S8)->position_2 = _S7.position_0;

#line 29
    (&_S8)->texcoord_1 = _S7.texcoord_0;

#line 29
    (&_S8)->color_2 = _S7.color_1;

#line 29
    return _S8;
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


#line 26
struct pixelInput_0
{
    float2 texcoord_0 [[user(TEXCOORD)]];
    float4 color_0 [[user(COLOR)]];
};


#line 33 "D:/Projects/mercury/engine/shaders/dedicated_sprite.slang"
[[fragment]] pixelOutput_0 DedicatedSpriteColorPS(pixelInput_0 _S1 [[stage_in]], float4 position_0 [[position]])
{

#line 33
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


#line 39
struct pixelInput_0
{
    float2 texcoord_0 [[user(TEXCOORD)]];
    float4 color_0 [[user(COLOR)]];
};


#line 39 "D:/Projects/mercury/engine/shaders/dedicated_sprite.slang"
[[fragment]] pixelOutput_0 DedicatedSpritePS(pixelInput_0 _S1 [[stage_in]], float4 position_0 [[position]])
{

#line 39
    pixelOutput_0 _S2 = { _S1.color_0 * float4(_S1.texcoord_0, 0.0, 1.0) };

    return _S2;
}

)";
	return { data, sizeof(data) };
}

mercury::ll::graphics::ShaderBytecodeView TestTriangleVS()
{
	static const char data[] = R"(#include <metal_stdlib>
#include <metal_math>
#include <metal_texture>
using namespace metal;

#line 33 "D:/Projects/mercury/engine/shaders/mercury_base.slang"
struct TestTriangleVS_Result_0
{
    float4 position_0 [[position]];
    float4 color_0 [[user(COLOR)]];
};


#line 33
struct BaseVertexOutputColorOnly_0
{
    float4 position_1;
    float4 color_1;
};


#line 33
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

mercury::ll::graphics::ShaderBytecodeView TestTriangleRotatedVS()
{
	static const char data[] = R"(#include <metal_stdlib>
#include <metal_math>
#include <metal_texture>
using namespace metal;

#line 33 "D:/Projects/mercury/engine/shaders/mercury_base.slang"
struct BaseVertexOutputColorOnly_0
{
    float4 position_0;
    float4 color_0;
};


#line 19 "D:/Projects/mercury/engine/shaders/mercury.slang"
struct TestTrianglePerObject_0
{
    float angle_0;
};


#line 19
struct EntryPointParams_0
{
    TestTrianglePerObject_0 perObject_0;
};


#line 19
struct KernelContext_0
{
    EntryPointParams_0 constant* entryPointParams_0;
};



BaseVertexOutputColorOnly_0 TestTriangleRotatedVS_0(const uint thread* vertexID_0, KernelContext_0 thread* kernelContext_0)
{


    array<float2, int(3)> positions_0 = { float2(0.0, 0.5), float2(0.5, -0.5), float2(-0.5, -0.5) };
    array<float4, int(3)> colors_0 = { float4(1.0, 0.0, 0.0, 1.0), float4(0.0, 1.0, 0.0, 1.0), float4(0.0, 0.0, 1.0, 1.0) };

    float cosAngle_0 = cos(kernelContext_0->entryPointParams_0->perObject_0.angle_0);
    float sinAngle_0 = sin(kernelContext_0->entryPointParams_0->perObject_0.angle_0);


    float _S1 = positions_0[*vertexID_0].x;

#line 37
    float _S2 = positions_0[*vertexID_0].y;

#line 28
    thread BaseVertexOutputColorOnly_0 output_0;

#line 40
    (&output_0)->position_0 = float4(float2(_S1 * cosAngle_0 - _S2 * sinAngle_0, _S1 * sinAngle_0 + _S2 * cosAngle_0), 0.0, 1.0);
    (&output_0)->color_0 = colors_0[*vertexID_0];

    return output_0;
}


#line 43
struct TestTriangleRotatedVS_Result_0
{
    float4 position_1 [[position]];
    float4 color_1 [[user(COLOR)]];
};


#line 43
[[vertex]] TestTriangleRotatedVS_Result_0 TestTriangleRotatedVS(uint vertexID_1 [[vertex_id]])
{

#line 43
    thread uint _S3 = vertexID_1;

#line 43
    thread KernelContext_0 kernelContext_1;

#line 43
    BaseVertexOutputColorOnly_0 _S4 = TestTriangleRotatedVS_0(&_S3, &kernelContext_1);

#line 43
    thread TestTriangleRotatedVS_Result_0 _S5;

#line 43
    (&_S5)->position_1 = _S4.position_0;

#line 43
    (&_S5)->color_1 = _S4.color_0;

#line 43
    return _S5;
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


#line 47 "D:/Projects/mercury/engine/shaders/mercury.slang"
[[fragment]] pixelOutput_0 TestTrianglePS(pixelInput_0 _S1 [[stage_in]], float4 position_0 [[position]])
{

#line 47
    pixelOutput_0 _S2 = { _S1.color_0 };

    return _S2;
}

)";
	return { data, sizeof(data) };
}

} // namespace mercury::ll::graphics::embedded_shaders
#endif