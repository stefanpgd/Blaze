#include "Common.hlsl"

// Raytracing output texture, accessed as a UAV
RWTexture2D<float4> gOutput : register(u0);
RWTexture2D<float4> colorBuffer : register(u1);

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0);

struct Settings
{
    float time;
    uint frameCount;
};
ConstantBuffer<Settings> settings : register(b0);

uint GetSeed(uint2 launchIndex)
{
    float2 dims = float2(DispatchRaysDimensions().xy);
    
    uint pixelIndex = launchIndex.x + launchIndex.y * dims.x;
    return (settings.frameCount * 26927) + (pixelIndex * 78713);
}

float3 GetRayDirection(inout uint seed, uint2 launchIndex, float3 cameraPosition)
{
    // 1) Get a random location within a given pixel //
    float stochasticX = Random01(seed);
    float stochasticY = Random01(seed);
    
    float2 dimensions = float2(DispatchRaysDimensions().xy);
    float2 uv = (launchIndex.xy + float2(stochasticX, stochasticY)) / dimensions.xy;
    
    // 2) Setup virtual screen plane //
    float aspectRatio = dimensions.x / dimensions.y;
    float xOffset = (aspectRatio - 1.0f) * 0.5f;
    
    float3 direction = float3(0.0f, 0.0f, -1.0f);
    float3 planeOffset = 2.0f;
    
    float3 screenCenter = cameraPosition + (direction * planeOffset);
    
    float3 screenP0 = screenCenter + float3(-0.5 - xOffset, 0.5, 0.0f);
    float3 screenP1 = screenCenter + float3(0.5 + xOffset, 0.5, 0.0f);
    float3 screenP2 = screenCenter + float3(-0.5 - xOffset, -0.5, 0.0f);
    
    float3 screenU = screenP1 - screenP0;
    float3 screenV = screenP2 - screenP0;
    
    // 3) Use the plane to find our given ray direction //
    float3 screenPoint = screenP0 + (screenU * uv.x) + (screenV * uv.y);
    float3 rayDirection = normalize(screenPoint - cameraPosition);
    
    return rayDirection;
}


[shader("raygeneration")]
void RayGen()
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint seed = GetSeed(launchIndex);

    float3 position = float3(0.0f, 0.0f, 7.5f);
    float3 rayDir = GetRayDirection(seed, launchIndex, position);
    
    RayDesc ray;
    ray.Origin = position;
    ray.Direction = rayDir;
    ray.TMin = 0.001f;
    ray.TMax = 100000;
    
    HitInfo payload;
    payload.depth = 0;
    payload.seed = seed;
    
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
    
    colorBuffer[launchIndex] += float4(payload.color, 1.0f);
    int sampleCount = colorBuffer[launchIndex].a;
    
    gOutput[launchIndex] = float4(colorBuffer[launchIndex].rgb / sampleCount, 1.0f);
}