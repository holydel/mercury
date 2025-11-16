#include "mercury_embedded_shaders.h"

#ifdef MERCURY_LL_GRAPHICS_WEBGPU

namespace mercury::ll::graphics::embedded_shaders {

mercury::ll::graphics::ShaderBytecodeView DedicatedSpriteVS()
{
	static const char data[] = R"(struct MercuryScene_std140_0
{
    @align(16) prerptationMatrix_0 : vec4<f32>,
    @align(16) canvasSize_0 : vec4<f32>,
    @align(16) time_0 : f32,
    @align(4) deltaTime_0 : f32,
};

@binding(0) @group(0) var<uniform> perFrame_0 : MercuryScene_std140_0;
struct DedicatedSpriteParameters_std140_0
{
    @align(16) position_0 : vec2<f32>,
    @align(8) size_0 : vec2<f32>,
    @align(16) uvs_0 : vec4<f32>,
    @align(16) angle_0 : f32,
    @align(4) colorPacked_0 : u32,
};

struct EntryPointParams_std140_0
{
    @align(16) perObject_0 : DedicatedSpriteParameters_std140_0,
};

@binding(0) @group(0) var<uniform> entryPointParams_0 : EntryPointParams_std140_0;
struct PackedColor_0
{
     packed_0 : u32,
};

fn PackedColor_x24init_0( packed_1 : u32) -> PackedColor_0
{
    var _S1 : PackedColor_0;
    _S1.packed_0 = packed_1;
    return _S1;
}

fn PackedColor_toFloat4_0( this_0 : PackedColor_0) -> vec4<f32>
{
    var color_0 : vec4<f32>;
    color_0[i32(0)] = f32(((((this_0.packed_0) >> (u32(24)))) & (u32(255)))) / 255.0f;
    color_0[i32(1)] = f32(((((this_0.packed_0) >> (u32(16)))) & (u32(255)))) / 255.0f;
    color_0[i32(2)] = f32(((((this_0.packed_0) >> (u32(8)))) & (u32(255)))) / 255.0f;
    color_0[i32(3)] = f32(((this_0.packed_0) & (u32(255)))) / 255.0f;
    return color_0;
}

struct BaseVertexOutput_0
{
    @builtin(position) position_1 : vec4<f32>,
    @location(0) texcoord_0 : vec2<f32>,
    @location(1) color_1 : vec4<f32>,
};

@vertex
fn main(@builtin(vertex_index) vertexID_0 : u32) -> BaseVertexOutput_0
{
    const _S2 : vec2<f32> = vec2<f32>(1.0f, 1.0f);
    var positions_ndc_0 : array<vec2<f32>, i32(4)> = array<vec2<f32>, i32(4)>( vec2<f32>(-1.0f, 1.0f), _S2, vec2<f32>(-1.0f, -1.0f), vec2<f32>(1.0f, -1.0f) );
    var uvs_1 : array<vec2<f32>, i32(4)> = array<vec2<f32>, i32(4)>( vec2<f32>(0.0f, 0.0f), vec2<f32>(1.0f, 0.0f), vec2<f32>(0.0f, 1.0f), _S2 );
    var cosAngle_0 : f32 = cos(entryPointParams_0.perObject_0.angle_0);
    var sinAngle_0 : f32 = sin(entryPointParams_0.perObject_0.angle_0);
    var output_0 : BaseVertexOutput_0;
    output_0.position_1 = vec4<f32>(((((positions_ndc_0[vertexID_0] * entryPointParams_0.perObject_0.size_0) * (mat2x2<f32>(cosAngle_0, - sinAngle_0, sinAngle_0, cosAngle_0)))) + entryPointParams_0.perObject_0.position_0) * perFrame_0.canvasSize_0.zw - vec2<f32>(1.0f, f32((i32(sign((perFrame_0.canvasSize_0.w)))))), 0.0f, 1.0f);
    output_0.texcoord_0 = uvs_1[vertexID_0];
    output_0.color_1 = PackedColor_toFloat4_0(PackedColor_x24init_0(entryPointParams_0.perObject_0.colorPacked_0));
    return output_0;
}

)";
	return { data, sizeof(data) };
}

mercury::ll::graphics::ShaderBytecodeView DedicatedSpriteColorPS()
{
	static const char data[] = R"(struct pixelOutput_0
{
    @location(0) output_0 : vec4<f32>,
};

struct pixelInput_0
{
    @location(0) texcoord_0 : vec2<f32>,
    @location(1) color_0 : vec4<f32>,
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

mercury::ll::graphics::ShaderBytecodeView DedicatedSpritePS()
{
	static const char data[] = R"(@binding(0) @group(1) var textureMap_texture_0 : texture_2d<f32>;

@binding(1) @group(1) var textureMap_sampler_0 : sampler;

struct pixelOutput_0
{
    @location(0) output_0 : vec4<f32>,
};

struct pixelInput_0
{
    @location(0) texcoord_0 : vec2<f32>,
    @location(1) color_0 : vec4<f32>,
};

@fragment
fn main( _S1 : pixelInput_0, @builtin(position) position_0 : vec4<f32>) -> pixelOutput_0
{
    ;
    var _S2 : pixelOutput_0 = pixelOutput_0( _S1.color_0 * (textureSample((textureMap_texture_0), (textureMap_sampler_0), (_S1.texcoord_0))) );
    return _S2;
}

)";
	return { data, sizeof(data) };
}

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

mercury::ll::graphics::ShaderBytecodeView TestTriangleRotatedVS()
{
	static const char data[] = R"(struct TestTrianglePerObject_std140_0
{
    @align(16) angle_0 : f32,
};

struct EntryPointParams_std140_0
{
    @align(16) perObject_0 : TestTrianglePerObject_std140_0,
};

@binding(0) @group(0) var<uniform> entryPointParams_0 : EntryPointParams_std140_0;
struct BaseVertexOutputColorOnly_0
{
    @builtin(position) position_0 : vec4<f32>,
    @location(0) color_0 : vec4<f32>,
};

@vertex
fn main(@builtin(vertex_index) vertexID_0 : u32) -> BaseVertexOutputColorOnly_0
{
    var positions_0 : array<vec2<f32>, i32(3)> = array<vec2<f32>, i32(3)>( vec2<f32>(0.0f, 0.5f), vec2<f32>(0.5f, -0.5f), vec2<f32>(-0.5f, -0.5f) );
    var colors_0 : array<vec4<f32>, i32(3)> = array<vec4<f32>, i32(3)>( vec4<f32>(1.0f, 0.0f, 0.0f, 1.0f), vec4<f32>(0.0f, 1.0f, 0.0f, 1.0f), vec4<f32>(0.0f, 0.0f, 1.0f, 1.0f) );
    var cosAngle_0 : f32 = cos(entryPointParams_0.perObject_0.angle_0);
    var sinAngle_0 : f32 = sin(entryPointParams_0.perObject_0.angle_0);
    var _S1 : f32 = positions_0[vertexID_0].x;
    var _S2 : f32 = positions_0[vertexID_0].y;
    var output_0 : BaseVertexOutputColorOnly_0;
    output_0.position_0 = vec4<f32>(vec2<f32>(_S1 * cosAngle_0 - _S2 * sinAngle_0, _S1 * sinAngle_0 + _S2 * cosAngle_0), 0.0f, 1.0f);
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