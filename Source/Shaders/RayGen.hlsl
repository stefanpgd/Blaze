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


float Random01(inout uint seed) 
{
    // XorShift32
    seed ^= (seed << 13);
    seed ^= (seed >> 17);
    seed ^= (seed << 5);
    return seed / 4294967296.0; 
}

float RandomInRange(inout uint seed, float min, float max)
{
    return min + (max - min) * Random01(seed);
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

float3 RandomUnitVector(inout uint seed)
{
    float x = RandomInRange(seed, -1.0f, 1.0f);
    float y = RandomInRange(seed, -1.0f, 1.0f);
    float z = RandomInRange(seed, -1.0f, 1.0f);
    
    float3 vec = float3(x, y, z);
    return normalize(vec);
}

[shader("raygeneration")]
void RayGen()
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    float2 dims = float2(DispatchRaysDimensions().xy);
    
    uint pixelIndex = launchIndex.x + launchIndex.y * dims.x;
    uint seed = (settings.frameCount * 26927) + (pixelIndex * 78713);
    
    // Initialize the ray payload
    HitInfo payload;
    payload.colorAndDistance = float4(0, 0, 0, 0);

    // Get the location within the dispatched 2D grid of work items
    // (often maps to pixels, so this could represent a pixel coordinate).
    
    float xOffset = Random01(seed);
    float yOffset = Random01(seed);
    
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
    float t = payload.colorAndDistance.a;
    
    if(t >= 0.0f)
    {
        // Do diffuse stuff //
        float3 intersection = ray.Origin = ray.Direction * t;
        float3 direction = RandomUnitVector(seed);
        
        if(dot(direction, payload.normal) < 0.0f)
        {
            direction = direction * -1.0f;
        }
        
        ray.Origin = intersection;
        ray.Direction = direction;
        
        float3 BRDF = color / PI; 
        float cosI = dot(payload.normal, direction);
        
        TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
        float3 irradiance = payload.colorAndDistance.rgb * cosI;
        
        color = PI * 2.0f * BRDF * irradiance;
    }
    
    float r = Random01(seed);
    
    colorBuffer[launchIndex] += float4(color, 1.0f);
    int sampleCount = colorBuffer[launchIndex].a;
    
    gOutput[launchIndex] = float4(colorBuffer[launchIndex].rgb / sampleCount, 1.0f);
}