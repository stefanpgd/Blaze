#include "Common.hlsl"

// Raytracing output texture, accessed as a UAV
RWTexture2D<float4> gOutput : register(u0);

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0);

float3 GetRayDirection(float normalizedX, float normalizedY)
{
    float3 position = float3(0.0f, 0.0f, 5.0f);
    float3 direction = float3(0.0f, 0.0f, -1.0f);
    float3 planeOffset = 1.0f;
    
    float3 screenCenter = position + (direction * planeOffset);
    
    float3 screenP0 = screenCenter + float3(-0.5, -0.5, 0.0f);
    float3 screenP1 = screenCenter + float3(0.5, -0.5, 0.0f);
    float3 screenP2 = screenCenter + float3(-0.5, 0.5, 0.0f);
    
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
    float2 uv = launchIndex.xy / dims.xy;

    // 1 - setup a screen place
    float3 position = float3(0.0f, 0.0f, 5.0f);
    float3 rayDir = GetRayDirection(uv.x, uv.y);
    
    
    RayDesc ray;
    ray.Origin = position;
    ray.Direction = rayDir;
    ray.TMin = 0;
    ray.TMax = 100000;
    
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
    
    gOutput[launchIndex] = float4(payload.colorAndDistance.rgb, 1.0f);
}