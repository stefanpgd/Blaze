#include "Graphics/EnvironmentMap.h"
#include "Graphics/Texture.h"
#include "Utilities/Logger.h"

#include <assert.h>
#include <tinyexr.h>

EnvironmentMap::EnvironmentMap(const std::string& filePath)
{
	const char* err = nullptr;
	float* image;

	int result = LoadEXR(&image, &width, &height, filePath.c_str(), &err);
	if(result != TINYEXR_SUCCESS)
	{
		std::string error(err);
		LOG(Log::MessageType::Error, "Failed to load EXR:");
		LOG(Log::MessageType::Error, error.c_str());

		assert(false);
	}

	environmentTexture = new Texture(image, width, height, DXGI_FORMAT_R32G32B32A32_FLOAT, sizeof(float) * 4);
	delete image;
}

Texture* EnvironmentMap::GetTexture()
{
	return environmentTexture;
}

DXGI_FORMAT EnvironmentMap::GetFormat()
{
	return environmentTexture->GetFormat();
}

ID3D12Resource* EnvironmentMap::GetAddress()
{
	return environmentTexture->GetResource().Get();
}
