#include "Common.hlsl"

// Raytracing output texture, accessed as a UAV
RWTexture2D<float4> gOutput : register(u0);
RWTexture2D<float4> colorBuffer : register(u1);

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0);

struct Settings
{
    float time;
};
ConstantBuffer<Settings> settings : register(b0);

float Random01(uint seed)
{
    // Hash function from H. Schechter & R. Bridson, goo.gl/RXiKaH
    seed ^= 2747636419u;
    seed *= 2654435769u;
    seed ^= seed >> 16;
    seed *= 2654435769u;
    seed ^= seed >> 16;
    seed *= 2654435769u;
    
    return float(seed) / 4294967295.0; // 2^32-1
}

float3 GetRayDirection(float normalizedX, float normalizedY)
{
    // Aspect Ratio //
    float2 dims = float2(DispatchRaysDimensions().xy);
    float aspectRatio = dims.x / dims.y;
    float xOffset = (aspectRatio - 1.0f) * 0.5f;
    
    float3 position = float3(0.0f, 0.0f, 5.0f);
    float3 direction = float3(0.0f, 0.0f, -1.0f);
    float3 planeOffset = 2.0f;
    
    float3 screenCenter = position + (direction * planeOffset);
    
    float3 screenP0 = screenCenter + float3(-0.5 - xOffset, 0.5, 0.0f);
    float3 screenP1 = screenCenter + float3(0.5 + xOffset, 0.5, 0.0f);
    float3 screenP2 = screenCenter + float3(-0.5 - xOffset, -0.5, 0.0f);
    
    float3 screenU = screenP1 - screenP0;
    float3 screenV = screenP2 - screenP0;
    
    float3 screenPoint = screenP0 + (screenU * normalizedX) + (screenV * normalizedY);
    
    float3 rayDirection = normalize(screenPoint - position);
    
    return rayDirection;
}

[shader("raygeneration")]
void RayGen()
{
  // Initialize the ray payload
    HitInfo payload;
    payload.colorAndDistance = float4(0, 0, 0, 0);

    // Get the location within the dispatched 2D grid of work items
    // (often maps to pixels, so this could represent a pixel coordinate).
    uint2 launchIndex = DispatchRaysIndex().xy;
    float2 dims = float2(DispatchRaysDimensions().xy);
    float2 d = (((launchIndex.xy + 0.5f) / dims.xy) * 2.f - 1.f);
    
    float xOffset = Random01((launchIndex.x + launchIndex.y * dims.x) * settings.time);
    float yOffset = Random01(((launchIndex.x + launchIndex.y * dims.x) + launchIndex.x) * settings.time);
    
    float2 uv = (launchIndex.xy + float2(xOffset, yOffset)) / dims.xy;

    // 1 - setup a screen place
    float3 position = float3(0.0f, 0.0f, 7.5f);
    float3 rayDir = GetRayDirection(uv.x, uv.y);
    
    RayDesc ray;
    ray.Origin = position;
    ray.Direction = rayDir;
    
    ray.TMin = 0;
    ray.TMax = 100000;
    
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
    
    float3 color = payload.colorAndDistance.rgb;
    
    const int maxBounces = 3;
    int bounces = 0;
    
    for(int i = 0; i < maxBounces; i++)
    {
        float t = payload.colorAndDistance.a;
        
        if(t <= 0.0f)
        {
            continue; // Don't bounce incase with hit the sky
        }

        float3 hitpoint = ray.Origin + ray.Direction * t;
        
        float3 newDir = reflect(ray.Direction, payload.normal);
        float3 newPos = hitpoint + newDir * 0.001f; // Epsilon..
        
        ray.Origin = newPos;
        ray.Direction = newDir;

        TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
        color *= payload.colorAndDistance.rgb;
    }
    
    colorBuffer[launchIndex] += float4(color, 1.0f);
    
    float noise = Random01((launchIndex.x + launchIndex.y * dims.x));
    //colorBuffer[launchIndex] = float4(xOffset, yOffset, 0.0f, 1.0f);
    int sampleCount = colorBuffer[launchIndex].a;
    
    gOutput[launchIndex] = float4(colorBuffer[launchIndex].rgb / sampleCount, 1.0f);
}