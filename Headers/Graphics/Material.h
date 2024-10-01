#pragma once

struct Material
{
	float color[3] = { 1.0f, 1.0f, 1.0f };
	float specularity = 0.0f;
	BOOL isEmissive = false;
	BOOL isDielectric = false;
	BOOL hasTextures = false;
	BOOL hasNormal = false;
	float stubs[56];
};