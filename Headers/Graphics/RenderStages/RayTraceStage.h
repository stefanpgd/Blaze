#pragma once

#include "Graphics/RenderStage.h"

class DXDescriptorHeap;

class RayTraceStage : public RenderStage
{
public:
	RayTraceStage(D3D12_GPU_VIRTUAL_ADDRESS tlasAddress);

	void RecordStage(ComPtr<ID3D12GraphicsCommandList4> commandList) override;

private:
	void CreateOutputBuffer();
	void CreateShaderResourceHeap(D3D12_GPU_VIRTUAL_ADDRESS tlasAddress);

private:
	ComPtr<ID3D12Resource> rayTraceOutput;
	DXDescriptorHeap* rayTraceHeap;
};