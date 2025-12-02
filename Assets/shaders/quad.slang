struct Vertex
{
    float4 position;
    float2 uv;
    float4 color;
    float texIndex;
    float tilingFactor;
    int entityID; 
}

struct PushConstants 
{
    float4x4 viewProjMatrix;
    Vertex *vertex;
}

[[vk::push_constant]]
PushConstants pc;

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv;
    float4 color;
    nointerpolation int texIndex;
    float tilingFactor;
    nointerpolation int entityID;
};

[shader("vertex")]
VSOutput mainVS(uint vertexID : SV_VertexID)
{
    VSOutput output;
    Vertex v = pc.vertex[vertexID];

    output.position = mul(v.position, pc.viewProjMatrix);
    output.uv = v.uv;
    output.color = v.color;
    output.texIndex = (int) v.texIndex;
    output.tilingFactor = v.tilingFactor;
    output.entityID = v.entityID;
    return output;
}

[[vk::binding(0, 1)]] Sampler2D texture[] : register(t0);

struct PSOutput
{
    float4 outColor : SV_Target0;
    int entityID : SV_Target1;
}

[shader("pixel")]
PSOutput mainPS(VSOutput input)
{
    PSOutput output;

    float4 texColor = texture[input.texIndex].Sample(input.uv * input.tilingFactor);

    output.outColor = texColor * input.color;
    output.entityID = input.entityID;
    return output;
}