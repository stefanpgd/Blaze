#include "Common.hlsl"

Texture2D<float4> environmentMap : register(t0);

[shader("miss")]
void Miss(inout HitInfo payload : SV_RayPayload)
{
    float y = (WorldRayDirection().y + 1.0f) * 0.5f;
    
    float3 a = float3(0.0f, 0.0f, 0.0f);
    float3 b = float3(1.0f, 1.0f, 1.0f);
    
    float3 result = lerp(a, b, y);
    
    // Use environment map //
    float3 rayDirection = WorldRayDirection();
    float theta = acos(rayDirection.y); 
    float phi = atan2(rayDirection.z, rayDirection.x) + PI;
    
    float u = phi / (2 * PI);
    float v = theta / PI;
    
    uint width;
    uint height;
    environmentMap.GetDimensions(width, height);
    
    int i = (int)(u * width);
    int j = (int)(v * height);
    
    i = i % width;
    j = j % height;
    float3 environmentSample = environmentMap[uint2(i, j)].rgb;
    
    environmentSample = clamp(environmentSample, float3(0.0f, 0.0f, 0.0f), float3(100.0f, 100.0f, 100.0f));
    
    payload.color = float3(environmentSample);
    payload.depth = 0.0f;
}