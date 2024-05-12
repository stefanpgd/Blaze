#include "Common.hlsl"

[shader("miss")]
void Miss(inout HitInfo payload : SV_RayPayload)
{
    float y = saturate(WorldRayDirection().y);
    
    float3 a = float3(0.25f, 0.25f, 0.25f);
    float3 b = float3(1.0f, 1.0f, 1.0f);
    
    float3 result = lerp(a, b, y);
    
    payload.colorAndDistance = float4(result, -1.f);
}