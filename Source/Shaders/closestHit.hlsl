#include "Common.hlsl"

struct Vertex
{
    float3 position;
    float2 uv;
    float3 normal;
};
StructuredBuffer<Vertex> VertexData : register(t0);
StructuredBuffer<int> indices : register(t1);
RaytracingAccelerationStructure SceneBVH : register(t2);
Texture2D<float4> diffuseTexture : register(t3);

struct Material
{
    float3 color;
    float specularity;
    bool isEmissive;
    bool isDielectric;
};
ConstantBuffer<Material> material : register(b0);

[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
    if(payload.depth > 0)
    {
        payload.color = 0.0;
        return;
    }
    
    const float3 intersection = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    
    uint vertID = PrimitiveIndex() * 3;
    Vertex a = VertexData[indices[vertID]];
    Vertex b = VertexData[indices[vertID + 1]];
    Vertex c = VertexData[indices[vertID + 2]];
    
    float3 baryCoords = float3(1.0f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
    float3 surfaceNormal = a.normal * baryCoords.x + b.normal * baryCoords.y + c.normal * baryCoords.z;
    surfaceNormal = normalize(mul(ObjectToWorld3x4(), float4(surfaceNormal, 0.0f)).xyz);
    
    float2 uv = a.uv * baryCoords.x + b.uv * baryCoords.y + c.uv * baryCoords.z;
    
    float3 sunDirection = normalize(float3(-0.4, 1, 0.5));
    float3 outputColor = 0.0f;
    
    // CAST SHADOW //
    const int shadowRayCount = 4;
    const float sunDiskSize = 0.025f;
    float shadowAccumalation = 0.0f;
    
    for(int i = 0; i < shadowRayCount; i++)
    {
        float sampleSunX = RandomInRange(payload.seed, -sunDiskSize, sunDiskSize);
        float sampleSunY = RandomInRange(payload.seed, -sunDiskSize, sunDiskSize);
        float3 rayD = normalize(sunDirection + float3(sampleSunX, sampleSunY, 0.0f));
        
        RayDesc shadowRay;
        shadowRay.Origin = intersection;
        shadowRay.Direction = rayD;
        shadowRay.TMin = 0.001f;
        shadowRay.TMax = 10000.0f;
        
        HitInfo shadowPayload;
        shadowPayload.depth = 1;
        TraceRay(SceneBVH, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 0, 0, 0, shadowRay, shadowPayload);
        
        if(!any(shadowPayload.color))
        {
            shadowAccumalation += 1.0f;
        }
    }
    
    int width;
    int height;
    diffuseTexture.GetDimensions(width, height);
    uint2 pixelLoc = uint2(uv.x * width, uv.y * height);
    float3 diffuseColor = diffuseTexture[pixelLoc].rgb;
    
    outputColor = diffuseColor * max(dot(surfaceNormal, sunDirection), 0.05);
    float shadow = 1.0 - (shadowAccumalation / shadowRayCount);
    shadow = max(shadow, 0.05f);
    
    payload.color = outputColor * shadow;
    return;
}