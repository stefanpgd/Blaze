#pragma once

#include <string>
#include "DXCommon.h"

class Texture;

class EnvironmentMap
{
public:
	EnvironmentMap(const std::string& filePath);

	Texture* GetTexture();

	DXGI_FORMAT GetFormat();
	ID3D12Resource* GetAddress();

private:
	Texture* environmentTexture;

	int width;
	int height;
};