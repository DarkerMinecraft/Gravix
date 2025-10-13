[vk::binding(0, 2)] RWTexture2D<float4> image : register(u0, s2);

struct PushConstants
{
    float4 topColor;
    float4 bottomColor;
};

[[vk::push_constant]]
PushConstants pc;

[numthreads(16, 16, 1)]
[shader("compute")]
void mainCS(uint3 globalID: SV_DispatchThreadID)
{
    int2 texelCoord = int2(globalID.xy);

    int2 size;
    image.GetDimensions(size.x, size.y);

    float4 topColor = pc.topColor;
    float4 bottomColor = pc.bottomColor;

    if (texelCoord.x < size.x && texelCoord.y < size.y)
    {
        float blend = float(texelCoord.y) / float(size.y);
        image[texelCoord] = lerp(topColor, bottomColor, blend);
    }
}
