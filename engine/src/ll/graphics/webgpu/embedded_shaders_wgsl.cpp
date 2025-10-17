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
fn TestTriangleVS(@builtin(vertex_index) vertexID_0 : u32) -> BaseVertexOutputColorOnly_0
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
fn TestTrianglePS( _S1 : pixelInput_0, @builtin(position) position_0 : vec4<f32>) -> pixelOutput_0
{
    var _S2 : pixelOutput_0 = pixelOutput_0( _S1.color_0 );
    return _S2;
}

)";
	return { data, sizeof(data) };
}

mercury::ll::graphics::ShaderBytecodeView DedicatedSpriteVS()
{
	static const char data[] = R"(struct PackedColor_std140_0
{
    @align(16) packed_0 : u32,
};

struct DedicatedSpriteParameters_std140_0
{
    @align(16) position_0 : vec2<f32>,
    @align(8) size_0 : vec2<f32>,
    @align(16) uvs_0 : vec4<f32>,
    @align(16) angle_0 : f32,
    @align(16) colorPacked_0 : PackedColor_std140_0,
};

struct EntryPointParams_std140_0
{
    @align(16) params_0 : DedicatedSpriteParameters_std140_0,
};

@binding(0) @group(0) var<uniform> entryPointParams_0 : EntryPointParams_std140_0;
struct MercuryScene_std140_0
{
    @align(16) canvasSize_0 : vec4<f32>,
};

@binding(0) @group(1) var<uniform> entryPointParams_scene_0 : MercuryScene_std140_0;
struct PackedColor_0
{
     packed_0 : u32,
};

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
fn DedicatedSpriteVS(@builtin(vertex_index) vertexID_0 : u32) -> BaseVertexOutput_0
{
    const _S1 : vec2<f32> = vec2<f32>(1.0f, 1.0f);
    var positions_ndc_0 : array<vec2<f32>, i32(4)> = array<vec2<f32>, i32(4)>( vec2<f32>(-1.0f, 1.0f), _S1, vec2<f32>(-1.0f, -1.0f), vec2<f32>(1.0f, -1.0f) );
    var uvs_1 : array<vec2<f32>, i32(4)> = array<vec2<f32>, i32(4)>( vec2<f32>(0.0f, 0.0f), vec2<f32>(1.0f, 0.0f), vec2<f32>(0.0f, 1.0f), _S1 );
    var output_0 : BaseVertexOutput_0;
    output_0.position_1 = vec4<f32>(positions_ndc_0[vertexID_0] * entryPointParams_scene_0.canvasSize_0.zw, 0.0f, 1.0f);
    output_0.texcoord_0 = uvs_1[vertexID_0];
    var _S2 : PackedColor_0 = PackedColor_0( entryPointParams_0.params_0.colorPacked_0.packed_0 );
    output_0.color_1 = PackedColor_toFloat4_0(_S2);
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
fn DedicatedSpriteColorPS( _S1 : pixelInput_0, @builtin(position) position_0 : vec4<f32>) -> pixelOutput_0
{
    var _S2 : pixelOutput_0 = pixelOutput_0( _S1.color_0 );
    return _S2;
}

)";
	return { data, sizeof(data) };
}

mercury::ll::graphics::ShaderBytecodeView DedicatedSpritePS()
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
fn DedicatedSpritePS( _S1 : pixelInput_0, @builtin(position) position_0 : vec4<f32>) -> pixelOutput_0
{
    var _S2 : pixelOutput_0 = pixelOutput_0( _S1.color_0 * vec4<f32>(_S1.texcoord_0, 0.0f, 1.0f) );
    return _S2;
}

)";
	return { data, sizeof(data) };
}

} // namespace mercury::ll::graphics::embedded_shaders
#endif