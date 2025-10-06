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
    return input.color + float4(0.1f,0.9f,0.1f,0.0f);
}

[shader("fragment")]
float4 TestTriangleRedPS(BaseVertexOutputColorOnly input) : SV_Target
{
    return input.color + float4(0.5f, 0.0f, 0.0f, 0.0f);
}

[shader("fragment")]
float4 TestTriangleGreenPS(BaseVertexOutputColorOnly input) : SV_Target
{
    return input.color + float4(-0.1f, 0.9f, -0.1f, 0.0f);
}

[shader("fragment")]
float4 TestTriangleStripplePS(BaseVertexOutputColorOnly input) : SV_Target
{
    float3 screenPos = input.position.xyz / input.position.w;

    if ((int(screenPos.x) & 1) == 0)
        discard;

    return float4((input.position.xyz / input.position.w) * 0.001,1.0f);
}

StructuredBuffer<float> buffer0;
StructuredBuffer<float> buffer1;
RWStructuredBuffer<float> result;

[shader("compute")]
[numthreads(128, 1, 1)]
void computeSimpleSumm(uint3 threadId: SV_DispatchThreadID)
{
    uint index = threadId.x;
    result[index] = buffer0[index] + buffer1[index];
}

[vk_image_format("rgba8")]
RWTexture2D<float4> outputImage;

[shader("compute")]
[numthreads(8, 8, 1)]
void computeSimpleImage(uint3 threadId: SV_DispatchThreadID)
{
    // Calculate pixel coordinates
    uint2 pixelCoord = threadId.xy;

    // Check if we're within image bounds (512x512)
    if (pixelCoord.x < 512 && pixelCoord.y < 512)
    {
        // Create a simple pattern based on position
        float2 uv = float2(pixelCoord) / 512.0;

        // Generate color pattern (example: gradient + checkboard)
        float3 color = float3(uv.x, uv.y, 0.5); // Basic gradient

        // Add a simple checkboard pattern
        uint2 tile = (pixelCoord / 8) % 2;
        float checker = (tile.x + tile.y) % 2;
        color *= 0.5 + 0.5 * checker;

        // Output the final color
        outputImage[pixelCoord] = float4(color, 1.0);
    }
}

/*
[shader("compute")]
[numthreads(8, 8, 1)]
void simpleComputeImage(uint3 groupID: SV_GroupID, uint3 threadID: SV_GroupThreadID, uint3 dispatchThreadID: SV_DispatchThreadID)
{
    // Calculate pixel coordinates
    uint2 pixelCoord = dispatchThreadID.xy;

    // Check if we're within image bounds (512x512)
    if (pixelCoord.x < 512 && pixelCoord.y < 512)
    {
        // Create a simple pattern based on position
        float2 uv = float2(pixelCoord) / 512.0;

        // Generate color pattern (example: gradient + checkboard)
        float3 color = float3(uv.x, uv.y, 0.5); // Basic gradient

        // Add a simple checkboard pattern
        uint2 tile = (pixelCoord / 8) % 2;
        float checker = (tile.x + tile.y) % 2;
        color *= 0.5 + 0.5 * checker;

        // Output the final color
        outputImage[pixelCoord] = float4(color, 1.0);
    }
}
*/