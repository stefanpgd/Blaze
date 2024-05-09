#pragma once

#include "Graphics/RenderStage.h"

class DXDescriptorHeap;
class DXRayTracingPipeline;

class RayTraceStage : public RenderStage
{
public:
	RayTraceStage(D3D12_GPU_VIRTUAL_ADDRESS tlasAddress);

	void RecordStage(ComPtr<ID3D12GraphicsCommandList4> commandList) override;
	
private:
	void CreateOutputBuffer();
	void CreateShaderResourceHeap(D3D12_GPU_VIRTUAL_ADDRESS tlasAddress);

	void InitializePipeline();

private:
	DXRayTracingPipeline* rayTracePipeline;

	ComPtr<ID3D12Resource> rayTraceOutput;
	DXDescriptorHeap* rayTraceHeap;

	D3D12_DISPATCH_RAYS_DESC dispatchRayDescription;
};