#pragma once

#include "Graphics/RenderStage.h"

class Mesh;
class DXDescriptorHeap;
class DXRayTracingPipeline;
class DXTopLevelAS;
class DXShaderBindingTable;
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
	void CreateShaderResources();
	void CreateShaderDescriptors();

	void InitializePipeline();
	void InitializeShaderBindingTable();

private:
	PipelineSettings settings;
	unsigned int rayGenTableIndex = 0;

	// Ray Tracing Components //
	DXTopLevelAS* TLAS;
	DXRayTracingPipeline* rayTracePipeline;
	DXShaderBindingTable* shaderTable;

	// Buffers for screen //
	Texture* outputBuffer;
	Texture* colorBuffer;

	Mesh* mesh;
	Scene* activeScene;
	ComPtr<ID3D12Resource> settingsBuffer;
};