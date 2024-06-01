#pragma once

#include "Graphics/DXCommon.h"

class Scene;

class DXTopLevelAS
{
public:
	DXTopLevelAS(Scene* scene);

	void RebuildTLAS();

	void SetScene(Scene* scene);
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress();

private:
	void BuildTLAS();

private:
	Scene* activeScene;

	ComPtr<ID3D12Resource> tlasInstanceDesc;
	ComPtr<ID3D12Resource> tlasScratch;
	ComPtr<ID3D12Resource> tlasResult;
};