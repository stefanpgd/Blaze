#pragma once

#include "Graphics/RenderStage.h"

class DXRayTracingPipeline;
class DXTopLevelAS;
class DXShaderBindingTable;
class DXUploadBuffer;
class Mesh;
class Texture;
class Scene;

struct PipelineSettings
{
	// TODO: Instead of placing stubs, update the util to actually
	// `ALIGN` the buffer, then just upload what data needs to be uploaded
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
	DXUploadBuffer* settingsBuffer;

	unsigned int rayGenTableIndex = 0;

	// Ray Tracing Components //
	DXTopLevelAS* TLAS;
	DXRayTracingPipeline* rayTracePipeline;
	DXShaderBindingTable* shaderTable;

	// Buffers for screen //
	Texture* outputBuffer;
	Texture* accumalationBuffer;

	Scene* activeScene;
};