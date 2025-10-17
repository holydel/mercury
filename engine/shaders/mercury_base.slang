module mercury_base;

public struct MercuryScene
{
    public float4 canvasSize; // xy = size, zw = 1/size
};

public struct PackedColor
{
    public uint32_t packed;
    public float4 toFloat4()
    {
        float4 color;
        color.r = ((packed >> 24) & 0xFF) / 255.0;
        color.g = ((packed >> 16) & 0xFF) / 255.0;
        color.b = ((packed >> 8) & 0xFF) / 255.0;
        color.a = (packed & 0xFF) / 255.0;
        return color;
    }
};

public struct BaseVertexOutput
{
    public float4 position : SV_Position;
    public float2 texcoord : TEXCOORD0;
    public float4 color : COLOR0;
};

public struct BaseVertexOutputColorOnly
{
    public float4 position : SV_Position;
    public float4 color : COLOR0;
};

public struct DedicatedSpriteParameters
{
    public float2 position;
    public float2 size;
    public float4 uvs;
    public float angle;
    public PackedColor colorPacked;    
};