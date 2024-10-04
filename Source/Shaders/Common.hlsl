// REGION - Buffers & Materials //
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

// REGION - Randomness //
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

// REGION - Utility Functions //
float Fresnel(float3 incoming, float3 normal, float IoR)
{
    float cosI = dot(incoming, normal);
    float n1 = 1.0f;
    float n2 = IoR;

    if (cosI > 0.0f)
    {
        float t = n1;
        n1 = n2;
        n2 = t;
    }

    float sinR = n1 / n2 * sqrt(max(1.0f - cosI * cosI, 0.0f));
    if (sinR >= 1.0f)
    {
		// TIR, aka perfect reflectance, which happens at the exact edges of a surface.
        return 1.0f;
    }

    float cosR = sqrt(max(1.0f - sinR * sinR, 0.0f));
    cosI = abs(cosI);

    float Fp = (n2 * cosI - n1 * cosR) / (n2 * cosI + n1 * cosR);
    float Fr = (n1 * cosI - n2 * cosR) / (n1 * cosI + n2 * cosR);

    return (Fp * Fp + Fr * Fr) * 0.5f;
}

float3 refract2(float3 incoming, float3 normal, float IoR)
{
    float cosI = dot(incoming, normal);
    float n1 = 1.0f;
    float n2 = IoR;
    float3 norm = normal;

    if (cosI < 0.0f)
    {
		// Going from air into medium
        cosI = -cosI;
    }
    else
    {
		// Going from medium back into air
        float t = n1;
        n1 = n2;
        n2 = t;
        norm = norm * -1.0f;
    }

    float eta = n1 / n2;
    float k = 1.0f - eta * eta * (1.0f - cosI * cosI);

    if (k < 0.0f)
    {
        return reflect(incoming, norm);
    }

    float3 a = eta * (incoming + (cosI * norm));
    float3 b = (norm * -1.0f) * sqrt(k);

    return a + b;
}