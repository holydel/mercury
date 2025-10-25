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

struct TestTrianglePerObject
{
    float angle;
}


[shader("vertex")]
BaseVertexOutputColorOnly TestTriangleRotatedVS(uniform TestTrianglePerObject perObject, uint vertexID : SV_VertexID)
{
    BaseVertexOutputColorOnly output;

    float2 positions[3] = { float2(0.0, 0.5), float2(0.5, -0.5), float2(-0.5, -0.5) };
    float4 colors[3] = { float4(1, 0, 0, 1), float4(0, 1, 0, 1), float4(0, 0, 1, 1) };

    float cosAngle = cos(perObject.angle);
    float sinAngle = sin(perObject.angle);
    float2 vpos = positions[vertexID];
    float2 rotatedPosition = float2(
        vpos.x * cosAngle - vpos.y * sinAngle,
        vpos.x * sinAngle + vpos.y * cosAngle
    );
    output.position = float4(rotatedPosition, 0.0, 1.0);
    output.color = colors[vertexID];

    return output;
}

[shader("fragment")]
float4 TestTrianglePS(BaseVertexOutputColorOnly input) : SV_Target
{
    return input.color;
}

