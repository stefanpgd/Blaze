#pragma once

// TODO:
// Do some bit-shifting/FLAG stuff and send that over to the GPU with a matching checker in Common.hlsl
struct Material
{
	float color[3] = { 1.0f, 1.0f, 1.0f };
	int materialType;
	float specularity;
	float IOR = 1.0f;
	float roughness = 0.0f;
	BOOL hasDiffuse = false;
	BOOL hasNormal = false;
	BOOL hasORM = false;
	float stubs[54];
};