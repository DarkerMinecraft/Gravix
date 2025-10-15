struct Vertex
{
    float4 position;
    float2 uv;
    float4 color;
    float texIndex;
    float tilingFactor;
}

struct PushConstants 
{
    float4x4 viewProjMatrix;
    Vertex *vertex;
}

[[vk::binding(0, 4)]]
ConstantBuffer<Vertex> _VertexStructReflection;

[[vk::push_constant]]
PushConstants pc;

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv;
    float4 color;
    int texIndex;
    float tilingFactor;
};

[shader("vertex")]
VSOutput mainVS(uint vertexID : SV_VertexID)
{
    VSOutput output;
    Vertex v = pc.vertex[vertexID];

    output.position = mul(pc.viewProjMatrix, v.position);
    output.uv = v.uv;
    output.color = v.color;
    output.texIndex = (int) v.texIndex;
    output.tilingFactor = v.tilingFactor;
    return output;
}

[[vk::binding(0, 1)]] Sampler2D texture[] : register(t0);

[shader("pixel")]
float4 mainPS(VSOutput input) : SV_TARGET
{
    float4 texColor = texture[input.texIndex].Sample(input.uv * input.tilingFactor);

    if (texColor.a * input.color.a < 0.2)
        discard;

    return texColor * input.color;
}