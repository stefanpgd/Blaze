#pragma once

// TODO:
// Do some bit-shifting/FLAG stuff and send that over to the GPU with a matching checker in Common.hlsl
struct Material
{
	float color[3] = { 1.0f, 1.0f, 1.0f };
	float specularity = 0.0f;
	BOOL isEmissive = false;
	BOOL isDielectric = false;
	BOOL hasDiffuse = false;
	BOOL hasNormal = false;
	BOOL hasORM = false;
	float stubs[55];
};