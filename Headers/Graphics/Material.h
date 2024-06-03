#pragma once

struct Material
{
	float color[3] = { 1.0f, 1.0f, 1.0f };
	float specularity = 0.0f;
	bool isEmissive = false;
	float stub[59];
};