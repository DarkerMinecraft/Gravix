struct Vertex
{
    float4 position;
    float2 uv;
    float4 color;
    float texIndex;
    float tilingFactor;
    uint entityID; 
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
    int texIndex;
    float tilingFactor;
    uint entityID;
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
    float4 outColor : SV_Target;
    uint entityID : SV_Target;
}

[shader("pixel")]
PSOutput mainPS(VSOutput input) : SV_TARGET
{
    PSOutput output; 

    float4 texColor = texture[input.texIndex].Sample(input.uv * input.tilingFactor);

    if (texColor.a * input.color.a < 0.2)
        discard;

    output.outColor = texColor * input.color;
    output.entityID = 50;

    return output;
}