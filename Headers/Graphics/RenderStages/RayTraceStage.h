#pragma once

#include "Graphics/RenderStage.h"

class Mesh;
class DXDescriptorHeap;
class DXRayTracingPipeline;
class DXTopLevelAS;

class RayTraceStage : public RenderStage
{
public:
	RayTraceStage(Mesh* mesh);

	void RecordStage(ComPtr<ID3D12GraphicsCommandList4> commandList) override;
	
private:
	void CreateOutputBuffer();
	void CreateColorBuffer();
	void CreateShaderResourceHeap();

	void InitializePipeline();

private:
	Mesh* mesh;

	DXRayTracingPipeline* rayTracePipeline;
	DXTopLevelAS* TLAS;

	ComPtr<ID3D12Resource> rayTraceOutput;
	ComPtr<ID3D12Resource> colorBuffer;
	DXDescriptorHeap* rayTraceHeap;

	D3D12_DISPATCH_RAYS_DESC dispatchRayDescription;
};