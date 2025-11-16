module dedicated_sprite;

import mercury_base;

[[vk::binding(0, 0)]]
ParameterBlock<MercuryScene> perFrame : register(b0, space0);

[[vk::binding(0, 1)]]
Sampler2D textureMap : register(t0, space2) : register(s0, space2);

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
    return input.color * textureMap.Sample(input.texcoord);
}
