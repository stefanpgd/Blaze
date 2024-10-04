#include "Common.hlsl"

struct Vertex
{
    float3 position;
    float3 normal;
    float3 tangent;
    float2 texCoord0;
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
    int materialType;
    float IOR;
    bool hasDiffuse;
    bool hasNormal;
    bool hasORM;
};
ConstantBuffer<Material> material : register(b0);

float3 ComputeConductorRadiance(float3 albedo, float3 normal, float3 currentDepth)
{
    float3 radiance = 0.0f;
    
    float3 intersection = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float3 direction = reflect(WorldRayDirection(), normal);
        
    RayDesc ray;
    ray.Origin = intersection;
    ray.Direction = direction;
    ray.TMin = 0.001f;
    ray.TMax = 100000;
        
    HitInfo reflectLoad;
    reflectLoad.depth = currentDepth;
        
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, reflectLoad);
    radiance += reflectLoad.color * albedo;
    
    return radiance;
}

float3 ComputeTransmissionRadiance(float3 albedo, float3 normal, float3 currentDepth)
{
    float3 radiance = 0.0f;
    
    float reflectance = Fresnel(WorldRayDirection(), normal, material.IOR);
    float transmittance = 1.0 - reflectance;
    float3 intersection = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
        
    if (reflectance > 0.0f)
    {
        float3 direction = reflect(WorldRayDirection(), normal);
        
        RayDesc ray;
        ray.Origin = intersection;
        ray.Direction = direction;
        ray.TMin = 0.001f;
        ray.TMax = 100000;
        
        HitInfo reflectLoad;
        reflectLoad.depth = currentDepth;
        
        TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, reflectLoad);
        radiance += reflectLoad.color * albedo * reflectance;
    }
        
    if (transmittance > 0.0f)
    {
        float3 direction = refract2(WorldRayDirection(), normal, material.IOR);
        
        RayDesc ray;
        ray.Origin = intersection;
        ray.Direction = direction;
        ray.TMin = 0.01f;
        ray.TMax = 100000;
        
        HitInfo refractLoad;
        refractLoad.depth = currentDepth;
        
        TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, refractLoad);
        radiance += refractLoad.color * albedo * transmittance;
    }
        
    return radiance;
}

float3 ComputeDielectricRadiance(float3 albedo, float3 normal, in HitInfo payload)
{
    float3 radiance = 0.0f;
 
    float3 intersection = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float specularFactor = saturate(Fresnel(WorldRayDirection(), normal, material.IOR));
    float diffuseFactor = 1.0 - specularFactor;
    
    if (diffuseFactor > 0.01)
    {
        float3 BRDF = albedo / PI;
    
        float3 direction = RandomUnitVector(payload.seed);
        if (dot(normal, direction) < 0.0f)
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
    
        HitInfo diffuseLoad;
        diffuseLoad.depth = payload.depth;
        
        TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, diffuseLoad);
        float3 irradiance = diffuseLoad.color * cosI;
    
        float3 diffuse = (PI * 2.0f * BRDF * irradiance) * diffuseFactor;
        radiance += diffuse;
    }
    
    if(specularFactor > 0.01)
    {
        float3 direction = reflect(WorldRayDirection(), normal);
        
        const float roughnessOffset = 0.3f;
        float3 randomInSphere = 0.0f;
        randomInSphere.x = RandomInRange(payload.seed, -roughnessOffset, roughnessOffset);
        randomInSphere.y = RandomInRange(payload.seed, -roughnessOffset, roughnessOffset);
        randomInSphere.z = RandomInRange(payload.seed, -roughnessOffset, roughnessOffset);
        
        direction += randomInSphere;
        
        RayDesc ray;
        ray.Origin = intersection;
        ray.Direction = direction;
        ray.TMin = 0.001f;
        ray.TMax = 100000;
        
        HitInfo reflectLoad;
        reflectLoad.depth = payload.depth;
        
        TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, reflectLoad);
        radiance += reflectLoad.color * albedo * specularFactor;
    }
    
    return radiance;
}

[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
    // Handle ray-tree depth //
    payload.depth += 1;
    const uint maxDepth = 6;

    if (payload.depth >= maxDepth)
    {
        payload.color = float3(0.0f, 0.0f, 0.0f);
        return;
    }
    
    // Vertex Data //
    uint vertID = PrimitiveIndex() * 3;
    Vertex a = VertexData[indices[vertID]];
    Vertex b = VertexData[indices[vertID + 1]];
    Vertex c = VertexData[indices[vertID + 2]];
    
    float3 baryCoords = float3(1.0f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);
    
    float3 normal = a.normal * baryCoords.x + b.normal * baryCoords.y + c.normal * baryCoords.z;
    float3 tangent = a.tangent * baryCoords.x + b.tangent * baryCoords.y + c.tangent * baryCoords.z;
    float2 uv = frac((a.texCoord0 * baryCoords.x) + (b.texCoord0 * baryCoords.y) + (c.texCoord0 * baryCoords.z));
    
    normal = normalize(mul(ObjectToWorld3x4(), float4(normal, 0.0f)).xyz);
    tangent = normalize(mul(ObjectToWorld3x4(), float4(tangent, 0.0f)).xyz);
    
    // Texture 
    int width;
    int height;
    diffuseTexture.GetDimensions(width, height);
    uint2 textureSampleLocation = uint2(uv.x * width, uv.y * height);
    
    float3 albedo = material.color;
    if (material.hasDiffuse)
    {
        albedo = diffuseTexture[textureSampleLocation].rgb * material.color;
    }
    
    if (material.hasNormal)
    {
        float3 biTangent = cross(normal, tangent);
        float3x3 TBN = float3x3(tangent, biTangent, normal);
        
        float3 n = (normalTexture[textureSampleLocation].rgb * 2.0) - float3(1.0, 1.0, 1.0);
        normal = normalize(mul(n, TBN));
    }
    
    // Calculate Radiance //
    float3 colorOutput = float3(0.0f, 0.0f, 0.0f);
    switch(material.materialType)
    {
        case 0: // Dielectric 
            colorOutput = ComputeDielectricRadiance(albedo, normal, payload);
            break;
        case 1: // Conductor
            colorOutput = ComputeConductorRadiance(albedo, normal, payload.depth);
            break;
        case 2: // Transmissive (Glass)
            colorOutput = ComputeTransmissionRadiance(albedo, normal, payload.depth);
            break;
        case 3: // Emissive 
            colorOutput = albedo;
            break;
    }
    
    payload.color = colorOutput;
}