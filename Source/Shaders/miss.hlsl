struct HitInfo
{
    float4 ShadedColorAndHitT;
};

// Miss shader gets called whenever the ray never intersected any geometry
// We get to implement what behaviour thus should occur, like returning the color of an HDR/EXR
// or in this case, a static color
[shader("miss")]
void Miss(inout HitInfo payload)
{
    payload.ShadedColorAndHitT = float4(0.2f, 0.2f, 0.2f, -1.f);
}