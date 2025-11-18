module dedicated_sprite;

import mercury_base;

#define MERCURY_DECLARE_TEXTURE2D(type, name) Texture2D<type> name##Map; SamplerState name##Sampler; type sample##name(float2 uv) { return name##Map.Sample(name##Sampler, uv); }

struct SpriteParameter
{
    MERCURY_DECLARE_TEXTURE2D(float4, Diffuse)
};

ParameterBlock<MercuryScene> perFrame;

ParameterBlock<SpriteParameter> spriteParams;

[shader("vertex")]
BaseVertexOutput DedicatedSpriteVS(uniform DedicatedSpriteParameters perObject, uint vertexID: SV_VertexID)
{
    BaseVertexOutput output;

    float2 positions_ndc[4] = { float2(-1.0, 1.0), float2(1.0, 1.0), float2(-1.0, -1.0), float2(1.0, -1.0) };
    float2 uvs[4] = { float2(0.0, 0.0), float2(1.0, 0.0), float2(0.0, 1.0), float2(1.0, 1.0) };

    float2 ndc = positions_ndc[vertexID];

    float cosAngle = cos(perObject.angle);
    float sinAngle = sin(perObject.angle);
    float2x2 rotationMatrix = float2x2(
        cosAngle, -sinAngle,
        sinAngle, cosAngle
    );
    ndc = mul(rotationMatrix,(ndc * perObject.size)) + perObject.position;

    output.position = float4(ndc * perFrame.canvasSize.zw - float2(1.0,sign(perFrame.canvasSize.w)), 0.0, 1.0);

    //output.position = float4(ndc, 0.0, 1.0);
    output.texcoord = uvs[vertexID];
    output.color = PackedColor(perObject.colorPacked).toFloat4();

    return output;
}

[shader("fragment")]
float4 DedicatedSpriteColorPS(BaseVertexOutput input) : SV_Target
{
    return input.color;
}

[shader("fragment")]
float4 DedicatedSpritePS(BaseVertexOutput input) : SV_Target
{
    return input.color * spriteParams.sampleDiffuse(input.texcoord);
}
