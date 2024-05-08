struct HitInfo
{
    float4 ShadedColorAndHitT;
};

// The Ray Generation Shader is the entry point of the RT pipeline
// It allows us to create a ray based on the constant data available ( such as camera info )
// Then call TraceRay(), which starts the rest of the pipeline
// Afterwards, a payload gets returned which allows us to output a color to the screen with the info received
[shader("raygeneration")]
void RayGen()
{
    uint2 launchIndex = DispatchRaysIndex().xy; // current Ray X-Y
    uint2 LaunchDimensions = DispatchRaysDimensions().xy; // screen size
    
    // Maps 0 - screenSize to -1 to 1
    float2 uv = (((launchIndex.xy + 0.5f) / uint2(1080, 720)) * 2.f - 1.f);
    
    RayDesc ray;
    ray.Origin = float3(uv, 0.0f);
    ray.Direction = float3(0.0f, 0.0f, -1.0f);
    ray.TMin = 0.1f;
    ray.TMax = 1000.0f;
    
    HitInfo payLoad;
    payLoad.ShadedColorAndHitT = float4(0.0f, 0.0f, 0.0f, 0.0f);

    // Call Trace Ray //
    
    // Output color to UAV texture
}