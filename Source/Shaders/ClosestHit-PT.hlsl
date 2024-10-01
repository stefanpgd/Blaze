#include "Common.hlsl"

struct Vertex
{
    float3 position;
    float2 uv;
    float3 normal;
    float3 tangent;
};
StructuredBuffer<Vertex> VertexData : register(t0);
StructuredBuffer<int> indices : register(t1);
RaytracingAccelerationStructure SceneBVH : register(t2);
Texture2D<float4> diffuseTexture : register(t3);
Texture2D<float4> normalTexture : register(t4);
Texture2D<float4> ormTexture : register(t5);

struct Material
{
    float3 color;
    float specularity;
    bool isEmissive;
    bool isDielectric;
    bool hasTextures;
    bool hasNormal;
    bool hasORM;
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

    float3 a = eta * (incoming + (cosI * norm));
    float3 b = (norm * -1.0f) * sqrt(k);

    return a + b;
}

[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
    int width;
    int height;
    diffuseTexture.GetDimensions(width, height);
    
    uint vertID = PrimitiveIndex() * 3;
    
    Vertex a = VertexData[indices[vertID]];
    Vertex b = VertexData[indices[vertID + 1]];
    Vertex c = VertexData[indices[vertID + 2]];
    
    float3 baryCoords = float3(1.0f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
    
    float3 normal = a.normal * baryCoords.x + b.normal * baryCoords.y + c.normal * baryCoords.z;
    normal = normalize(mul(ObjectToWorld3x4(), float4(normal, 0.0f)).xyz);
    
    float2 uv = frac((a.uv * baryCoords.x) + (b.uv * baryCoords.y) + (c.uv * baryCoords.z));
    uint2 textureSampleLocation = uint2(uv.x * width, uv.y * height);
    
    float3 materialColor = material.color;
    float alpha = 1.0;
    
    if(material.hasTextures)
    {
        materialColor = diffuseTexture[textureSampleLocation].rgb;
        alpha = diffuseTexture[textureSampleLocation].a;
    }
    
    if(material.hasNormal)
    {
        float3 tangent = a.tangent * baryCoords.x + b.tangent * baryCoords.y + c.tangent * baryCoords.z;
        tangent = normalize(mul(ObjectToWorld3x4(), float4(tangent, 0.0f)).xyz);
        
        float3 biTangent = cross(normal, tangent);
        float3x3 TBN = float3x3(tangent, biTangent, normal);
        
        float3 n = (normalTexture[textureSampleLocation].rgb * 2.0) - float3(1.0, 1.0, 1.0);
        normal = normalize(mul(n, TBN));
    }
    
    if(material.hasORM)
    {
        // stub //
    }
        
    payload.depth += 1;
    const uint maxDepth = 6;
    
    if(payload.depth >= maxDepth)
    {
        payload.color = float3(0.0f, 0.0f, 0.0f);
        return;
    }

    float3 colorOutput = float3(0.0f, 0.0f, 0.0f);
    
    if(material.isEmissive)
    {
        payload.color = materialColor;
        return;
    }
    
    if(material.isDielectric || alpha < 1.0)
    {
        float reflectance = Fresnel(WorldRayDirection(), normal, 1.51f);
        float transmittance = 1.0 - reflectance;
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
            colorOutput += reflectLoad.color * materialColor * reflectance;
        }
        
        if(transmittance > 0.0f)
        {
            float3 direction = refract2(WorldRayDirection(), normal, 1.51f);
        
            RayDesc ray;
            ray.Origin = intersection;
            ray.Direction = direction;
            ray.TMin = 0.01f;
            ray.TMax = 100000;
        
            HitInfo refractLoad;
            refractLoad.depth = payload.depth;
        
            TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, refractLoad);
            colorOutput += refractLoad.color * materialColor * transmittance;
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
        colorOutput += reflectLoad.color * material.specularity * materialColor;
    }
    
    // Surface we hit is 'Diffuse' so we scatter //
    if(1.0f - material.specularity > 0.01f && alpha >= 0.99)
    {        
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