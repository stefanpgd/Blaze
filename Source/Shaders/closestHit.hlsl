#include "Common.hlsl"

[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
    // The input received is a float2 representing barycentric coordinates u & v
    // where:
    // x = 1 - u - v
    // y = u
    // z = v
    
    float3 a = float3(1, 0, 0);
    float3 b = float3(0, 1, 0);
    float3 c = float3(0, 0, 1);
    
    float3 hit = a * (1.0 - attrib.bary.x - attrib.bary.y) + b * attrib.bary.x + c * attrib.bary.y;
    
    payload.colorAndDistance = float4(hit, RayTCurrent());
}