struct Vertex
{
    float3 position;
    float4 color; 
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
    float4 color;
};

[shader("vertex")]
VSOutput mainVS(uint vertexID : SV_VertexID)
{
    VSOutput output;
    Vertex v = pc.vertex[vertexID];

    output.position = mul(float4(v.position, 1.0), pc.viewProjMatrix);
    output.color = v.color;
    return output;
}

[shader("pixel")]
float4 mainPS(VSOutput input)
{
    return input.color;
}