module dedicated_mesh;

import mercury_base;

#define MERCURY_DECLARE_TEXTURE2D(type, name) Texture2D<type> name##Map; SamplerState name##Sampler; type sample##name(float2 uv) { return name##Map.Sample(name##Sampler, uv); }

struct StaticMeshParameter
{
    MERCURY_DECLARE_TEXTURE2D(float4, Diffuse)
};

ParameterBlock<MercuryScene> perFrame;

ParameterBlock<StaticMeshParameter> meshParams;

[shader("vertex")]
BaseVertexOutput DedicatedStaticMeshVS(uniform float4x4 MVP, DedicatedStaticMeshVertex vtx)
{
    BaseVertexOutput output;

    output.color = vtx.color;
    output.texcoord = vtx.uv0;
    output.position = mul(float4(vtx.position, 1.0), MVP);
    return output;
}

[shader("fragment")]
float4 DedicatedStaticMeshColorPS(BaseVertexOutput input) : SV_Target
{
    return input.color;
}

[shader("fragment")]
float4 DedicatedStaticMeshPS(BaseVertexOutput input) : SV_Target
{
    return input.color;
}