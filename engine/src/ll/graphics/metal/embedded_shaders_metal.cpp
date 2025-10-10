#include "mercury_embedded_shaders.h"

#ifdef MERCURY_LL_GRAPHICS_METAL

namespace mercury::ll::graphics::embedded_shaders {

mercury::ll::graphics::ShaderBytecodeView TestTriangleVS()
{
	static const char data[] = R"(#include <metal_stdlib>
#include <metal_math>
#include <metal_texture>
using namespace metal;

#line 10 "D:/Projects/mercury/engine/shaders/mercury_base.slang"
struct TestTriangleVS_Result_0
{
    float4 position_0 [[position]];
    float4 color_0 [[user(COLOR)]];
};


#line 10
struct BaseVertexOutputColorOnly_0
{
    float4 position_1;
    float4 color_1;
};


#line 10
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

} // namespace mercury::ll::graphics::embedded_shaders
#endif