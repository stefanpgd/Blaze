#pragma once

#include <d3d12.h>
#include <d3dx12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXStructuredBuffer
{
public:
	DXStructuredBuffer(const void* data, unsigned int numberOfElements, unsigned int elementSize);

	void UpdateData(const void* data);

	ComPtr<ID3D12Resource> GetResource();
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSRV();
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetUAV();

private:
	unsigned int numberOfElements;
	unsigned int elementSize;
	unsigned int bufferSize;

	ComPtr<ID3D12Resource> structuredBuffer;

	int srvIndex = -1;
	int uavIndex = -1;
};