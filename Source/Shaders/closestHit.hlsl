#include "Common.hlsl"

struct Vertex
{
    float3 position;
    float2 uv;
};
StructuredBuffer<Vertex> VertexData : register(t0);

[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
    uint vertID = PrimitiveIndex() * 3;
    Vertex a = VertexData[vertID];
    Vertex b = VertexData[vertID + 1];
    Vertex c = VertexData[vertID + 2];
    
    float3 baryCoords = float3(1.0f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
    float2 uv = a.uv * baryCoords.x + b.uv * baryCoords.y + c.uv * baryCoords.z;
    
    payload.colorAndDistance = float4(uv, 0.0f, RayTCurrent());
}