module mercury;

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