#include "Common.hlsl"

struct Vertex
{
    float3 position;
    float2 uv;
};
StructuredBuffer<Vertex> VertexData : register(t0);
StructuredBuffer<uint> indices : register(t1);

// StructuredBuffer(s) can be send as SRVs, since it's just a general resource
// The nice thing is that we can index this data ourself, either using information like primitive index
// Our with meshes / models, we could even consider using InstanceID()

[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
    uint vertID = PrimitiveIndex() * 3;
    
    Vertex a = VertexData[indices[vertID]];
    Vertex b = VertexData[indices[vertID + 1]];
    Vertex c = VertexData[indices[vertID + 2]];
    
    float3 baryCoords = float3(1.0f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
    float2 uv = a.uv * baryCoords.x + b.uv * baryCoords.y + c.uv * baryCoords.z;
    
    payload.colorAndDistance = float4(uv, 0.0f, RayTCurrent());
}