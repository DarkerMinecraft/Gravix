struct Vertex
{
    float4 worldPosition;
    float4 localPosition;
    float4 color;
    float thickness;
    float fade;
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
    float4 worldPosition : SV_POSITION;
    float4 localPosition;
    float4 color;
    float thickness;
    float fade;
    nointerpolation int entityID;
};

[shader("vertex")]
VSOutput mainVS(uint vertexID : SV_VertexID)
{
    VSOutput output;
    Vertex v = pc.vertex[vertexID];

    output.worldPosition = mul(v.worldPosition, pc.viewProjMatrix);
    output.localPosition = v.localPosition;
    output.color = v.color;
    output.thickness = v.thickness;
    output.fade = v.fade;
    output.entityID = v.entityID;
    return output;
}

struct PSOutput
{
    float4 outColor : SV_Target0;
    int entityID : SV_Target1;
}

[shader("pixel")]
PSOutput mainPS(VSOutput input)
{
    PSOutput output;

    float dist = 1 - length(input.localPosition.xy);
    float alphaColor = smoothstep(0.0, input.fade, dist);
    alphaColor *= smoothstep(input.thickness + input.fade, input.thickness, dist);

    float4 outColor = input.color;
    outColor.a *= alphaColor;

    output.outColor = outColor;

    if(alphaColor > 0.0)
        output.entityID = input.entityID;
    else
        output.entityID = -1;
    return output;
}