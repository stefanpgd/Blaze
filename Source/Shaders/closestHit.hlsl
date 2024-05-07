struct HitInfo
{
    float4 ShadedColorAndHitT;
};

// TODO: read into attributes for this shader and how they are supposed to work
struct Attributes
{
    float2 uv;
};

// The closet Hit shader's purpose is to be able to either shade the payload with a given color
// or allow us to call TraceRay() again with a newly generated ray
[shader("closesthit")]
void ClosestHit(inout HitInfo payload, in Attributes attrib)
{
	payload.ShadedColorAndHitT = float4(1.0f, 1.0f, 1.0f, 1.0f);
}