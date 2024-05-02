#pragma once

#include <d3d12.h>
#include <d3dx12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class Texture
{
public:
	Texture(void* data, int width, int height, 
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, unsigned int formatSizeInBytes = 4);
	~Texture();

	int GetWidth();
	int GetHeight();

	int GetSRVIndex();
	int GetUAVIndex();
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSRV();
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetUAV();

	DXGI_FORMAT GetFormat();
	ComPtr<ID3D12Resource> GetResource();
	D3D12_GPU_VIRTUAL_ADDRESS GetGPULocation();

private:
	void UploadData(void* data, int width, int height);

private:
	ComPtr<ID3D12Resource> textureResource;

	DXGI_FORMAT format;
	unsigned int formatSizeInBytes;
	int width;
	int height;

	int srvIndex = 0;
	int uavIndex = 0;
};