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

[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
    uint vertID = PrimitiveIndex() * 3;
    
    Vertex a = VertexData[indices[vertID]];
    Vertex b = VertexData[indices[vertID + 1]];
    Vertex c = VertexData[indices[vertID + 2]];
    
    float3 baryCoords = float3(1.0f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
    float3 normal = a.normal * baryCoords.x + b.normal * baryCoords.y + c.normal * baryCoords.z;
    normal = normalize(mul(ObjectToWorld3x4(), float4(normal, 0.0f)).xyz);
    
    payload.depth += 1;
    const uint maxDepth = 12;
    
    if(payload.depth >= maxDepth)
    {
        payload.color = float3(0.0f, 0.0f, 0.0f);
        return;
    }

    float3 colorOutput = float3(0.0f, 0.0f, 0.0f);
    if(InstanceID() == 0)
    {
        colorOutput = float3(1, 0.549, 0);
    }
    else if(InstanceID() == 1)
    {
        colorOutput = float3(1.0f, 1.0f, 1.0f);
      
        //colorOutput = float3(1.0f, 1.0f, 1.0f);
        //
        //// Metallic
        //float3 intersection = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
        //float3 direction = reflect(WorldRayDirection(), normal);
        //
        //RayDesc ray;
        //ray.Origin = intersection;
        //ray.Direction = direction;
        //ray.TMin = 0.001f;
        //ray.TMax = 100000;
        //
        //HitInfo reflectLoad;
        //reflectLoad.depth = payload.depth;
        //
        //TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, reflectLoad);
        //payload.color = reflectLoad.color;
        //return;
    }
    else
    {
        colorOutput = float3(1.0f, 1.0f, 1.0f);
    }
    
    // Surface we hit is 'Diffuse' so we scatter //
    float3 materialColor = colorOutput;
    float3 BRDF = materialColor / PI; // TODO: Read that article Jacco send about the distribution by Pi
    
    float3 intersection = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float3 direction = RandomUnitVector(payload.seed);
    if(dot(normal, direction) < 0.0f)
    {
        // Normal is facing inwards //
        direction *= -1.0f;
    }
    
    float cosI = dot(normal, direction);
    
    RayDesc ray;
    ray.Origin = intersection;
    ray.Direction = direction;
    ray.TMin = 0.001f;
    ray.TMax = 100000;
    
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
    float3 irradiance = payload.color * cosI;
    
    payload.color = PI * 2.0f * BRDF * irradiance;
}