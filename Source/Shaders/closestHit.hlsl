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
    uint vertID = PrimitiveIndex() * 3;
    Vertex a = VertexData[indices[vertID]];
    Vertex b = VertexData[indices[vertID + 1]];
    Vertex c = VertexData[indices[vertID + 2]];
    
    float3 baryCoords = float3(1.0f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
    float3 surfaceNormal = a.normal * baryCoords.x + b.normal * baryCoords.y + c.normal * baryCoords.z;
    surfaceNormal = normalize(mul(ObjectToWorld3x4(), float4(surfaceNormal, 0.0f)).xyz);
    
    payload.color = material.color;
}