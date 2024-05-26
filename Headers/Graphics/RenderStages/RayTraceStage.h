#pragma once

#include "Graphics/RenderStage.h"

class Mesh;
class DXDescriptorHeap;
class DXRayTracingPipeline;
class DXTopLevelAS;
class Texture;
class Scene;

struct ApplicationInfo;

class RayTraceStage : public RenderStage
{
public:
	RayTraceStage(Scene* scene, ApplicationInfo& applicationInfo);

	void Update();

	void RecordStage(ComPtr<ID3D12GraphicsCommandList4> commandList) override;
	
private:
	void CreateOutputBuffer();
	void CreateColorBuffer();
	void CreateShaderResourceHeap();

	void InitializePipeline();

private:
	ApplicationInfo& applicationInfo;

	Scene* activeScene;
	Mesh* mesh;
	Texture* EXRTexture;

	DXRayTracingPipeline* rayTracePipeline;
	DXTopLevelAS* TLAS;

	ComPtr<ID3D12Resource> rayTraceOutput;
	ComPtr<ID3D12Resource> colorBuffer;
	ComPtr<ID3D12Resource> appInfoBuffer;
	DXDescriptorHeap* rayTraceHeap;

	D3D12_DISPATCH_RAYS_DESC dispatchRayDescription;
};