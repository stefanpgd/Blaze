struct HitInfo
{
    float3 color;
    float depth;
    uint seed;
};

// Attributes output by the raytracing when hitting a surface,
// in this instance the barycentric coordinates
struct Attributes
{
    float2 bary;
};

static float PI = 3.14159265;

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

float3 RandomUnitVector(inout uint seed)
{
    float x = RandomInRange(seed, -1.0f, 1.0f);
    float y = RandomInRange(seed, -1.0f, 1.0f);
    float z = RandomInRange(seed, -1.0f, 1.0f);
    
    float3 vec = float3(x, y, z);
    return normalize(vec);
}