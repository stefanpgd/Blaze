struct HitInfo
{
    float4 colorAndDistance;
    float3 normal;
};

// Attributes output by the raytracing when hitting a surface,
// here the barycentric coordinates
struct Attributes
{
    float2 bary;
};

static float PI = 3.14159265;