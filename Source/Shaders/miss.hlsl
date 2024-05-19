#include "Common.hlsl"

[shader("miss")]
void Miss(inout HitInfo payload : SV_RayPayload)
{
    float y = (WorldRayDirection().y + 1.0f) * 0.5f;
    
    float3 a = float3(0.0f, 0.0f, 0.0f);
    float3 b = float3(1.0f, 1.0f, 1.0f);
    
    float3 result = lerp(a, b, y);
    
    payload.colorAndDistance = float4(result, -1.f);
}