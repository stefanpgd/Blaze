#pragma once

struct Material
{
	float color[3] = { 1.0f, 1.0f, 1.0f };
	bool isSpecular = false;
	float stub[60];
};