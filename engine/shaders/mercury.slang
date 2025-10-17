module mercury;

import mercury_base;

[shader("vertex")]
BaseVertexOutputColorOnly TestTriangleVS(uint vertexID : SV_VertexID)
{
    BaseVertexOutputColorOnly output;

    float2 positions[3] = { float2(0.0, 0.5), float2(0.5, -0.5), float2(-0.5, -0.5) };
    float4 colors[3] = { float4(1, 0, 0, 1), float4(0, 1, 0, 1), float4(0, 0, 1, 1) };

    output.position = float4(positions[vertexID], 0.0, 1.0);
    output.color = colors[vertexID];

    return output;
}

[shader("fragment")]
float4 TestTrianglePS(BaseVertexOutputColorOnly input) : SV_Target
{
    return input.color;
}

[shader("vertex")]
BaseVertexOutput DedicatedSpriteVS(uniform DedicatedSpriteParameters params, uniform ParameterBlock<MercuryScene> scene, uint vertexID: SV_VertexID)
{
    BaseVertexOutput output;

    float2 positions_ndc[4] = { float2(-1.0, 1.0), float2(1.0, 1.0), float2(-1.0, -1.0), float2(1.0, -1.0) };
    float2 uvs[4] = { float2(0.0, 0.0), float2(1.0, 0.0), float2(0.0, 1.0), float2(1.0, 1.0) };

    float2 ndc = positions_ndc[vertexID];
    output.position = float4(ndc * scene.canvasSize.zw, 0.0, 1.0);
    output.texcoord = uvs[vertexID];
    output.color = params.colorPacked.toFloat4();

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
    return input.color * float4(input.texcoord, 0.0, 1.0);
}