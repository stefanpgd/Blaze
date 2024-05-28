#pragma once

#include "Graphics/RenderStage.h"

class Mesh;
class DXDescriptorHeap;
class DXRayTracingPipeline;
class DXTopLevelAS;
class Texture;
class Scene;

struct PipelineSettings
{
	bool clearBuffers = false;
	float time = 1.0f;
	unsigned int frameCount = 0;
	float stub[61];
};

class RayTraceStage : public RenderStage
{
public:
	RayTraceStage(Scene* scene);

	void Update(float deltaTime);

	void RecordStage(ComPtr<ID3D12GraphicsCommandList4> commandList) override;
	
private:
	void CreateOutputBuffer();
	void CreateColorBuffer();
	void CreateShaderResourceHeap();

	void InitializePipeline();

private:
	Mesh* mesh;
	Scene* activeScene;
	PipelineSettings settings;

	DXRayTracingPipeline* rayTracePipeline;
	DXTopLevelAS* TLAS;

	ComPtr<ID3D12Resource> rayTraceOutput;
	ComPtr<ID3D12Resource> colorBuffer;
	ComPtr<ID3D12Resource> settingsBuffer;
	DXDescriptorHeap* rayTraceHeap;

	D3D12_DISPATCH_RAYS_DESC dispatchRayDescription;
};