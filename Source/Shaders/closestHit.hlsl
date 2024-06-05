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
};
ConstantBuffer<Material> material : register(b0);

float Fresnel(float3 incoming, float3 normal, float IoR)
{
    float cosI = dot(incoming, normal);
    float n1 = 1.0f;
    float n2 = IoR;

    if(cosI > 0.0f)
    {
        float t = n1;
        n1 = n2;
        n2 = t;
    }

    float sinR = n1 / n2 * sqrt(max(1.0f - cosI * cosI, 0.0f));
    if(sinR >= 1.0f)
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

    if(cosI < 0.0f)
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

    if(k < 0.0f)
    {
        return reflect(incoming, norm);
    }

    float3 a = eta * (incoming +(cosI * norm));
    float3 b = (norm * -1.0f) * sqrt(k);

    return a + b;
}

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
    const uint maxDepth = 10;
    
    if(payload.depth >= maxDepth)
    {
        payload.color = float3(0.0f, 0.0f, 0.0f);
        return;
    }

    float3 colorOutput = float3(0.0f, 0.0f, 0.0f);
    
    if(material.isEmissive)
    {
        payload.color = material.color;
        return;
    }
    
    {
        // DIELECTRIC
        float reflectance = Fresnel(WorldRayDirection(), normal, 1.51f);
        float transmittance = 1.0f - reflectance;
        float3 intersection = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
        
        if(reflectance > 0.0f)
        {
            float3 direction = reflect(WorldRayDirection(), normal);
        
            RayDesc ray;
            ray.Origin = intersection;
            ray.Direction = direction;
            ray.TMin = 0.001f;
            ray.TMax = 100000;
        
            HitInfo reflectLoad;
            reflectLoad.depth = payload.depth;
        
            TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, reflectLoad);
            colorOutput += reflectLoad.color * material.color * reflectance;
        }
        
        if(transmittance > 0.0f)
        {
            float3 direction = refract2(WorldRayDirection(), normal, 1.51f);
        
            RayDesc ray;
            ray.Origin = intersection;
            ray.Direction = direction;
            ray.TMin = 1.0f;
            ray.TMax = 100000;
        
            HitInfo refractLoad;
            refractLoad.depth = payload.depth;
        
            TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, refractLoad);
            colorOutput += refractLoad.color * material.color * transmittance;
        }
        
        payload.color = colorOutput;
        return;
    }
    
    if(material.specularity > 0.01f)
    {
        float3 intersection = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
        float3 direction = reflect(WorldRayDirection(), normal);
        
        RayDesc ray;
        ray.Origin = intersection;
        ray.Direction = direction;
        ray.TMin = 0.001f;
        ray.TMax = 100000;
        
        HitInfo reflectLoad;
        reflectLoad.depth = payload.depth;
        
        TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, reflectLoad);
        colorOutput += reflectLoad.color * material.specularity * material.color;
    }
    
    // Surface we hit is 'Diffuse' so we scatter //
    if(1.0f - material.specularity > 0.01f)
    {
        float3 materialColor = material.color;
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
    
        float3 diffuse = (PI * 2.0f * BRDF * irradiance) * (1.0f - material.specularity);
        colorOutput += diffuse;
    }
    
    payload.color = colorOutput;
}