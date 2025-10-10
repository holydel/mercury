#include "mercury_embedded_shaders.h"

#ifdef MERCURY_LL_GRAPHICS_WEBGPU

namespace mercury::ll::graphics::embedded_shaders {

mercury::ll::graphics::ShaderBytecodeView TestTriangleVS()
{
	static const char data[] = R"(struct BaseVertexOutputColorOnly_0
{
    @builtin(position) position_0 : vec4<f32>,
    @location(0) color_0 : vec4<f32>,
};

@vertex
fn main(@builtin(vertex_index) vertexID_0 : u32) -> BaseVertexOutputColorOnly_0
{
    var positions_0 : array<vec2<f32>, i32(3)> = array<vec2<f32>, i32(3)>( vec2<f32>(0.0f, 0.5f), vec2<f32>(0.5f, -0.5f), vec2<f32>(-0.5f, -0.5f) );
    var colors_0 : array<vec4<f32>, i32(3)> = array<vec4<f32>, i32(3)>( vec4<f32>(1.0f, 0.0f, 0.0f, 1.0f), vec4<f32>(0.0f, 1.0f, 0.0f, 1.0f), vec4<f32>(0.0f, 0.0f, 1.0f, 1.0f) );
    var output_0 : BaseVertexOutputColorOnly_0;
    output_0.position_0 = vec4<f32>(positions_0[vertexID_0], 0.0f, 1.0f);
    output_0.color_0 = colors_0[vertexID_0];
    return output_0;
}

)";
	return { data, sizeof(data) };
}

mercury::ll::graphics::ShaderBytecodeView TestTrianglePS()
{
	static const char data[] = R"(struct pixelOutput_0
{
    @location(0) output_0 : vec4<f32>,
};

struct pixelInput_0
{
    @location(0) color_0 : vec4<f32>,
};

@fragment
fn main( _S1 : pixelInput_0, @builtin(position) position_0 : vec4<f32>) -> pixelOutput_0
{
    var _S2 : pixelOutput_0 = pixelOutput_0( _S1.color_0 );
    return _S2;
}

)";
	return { data, sizeof(data) };
}

} // namespace mercury::ll::graphics::embedded_shaders
#endif