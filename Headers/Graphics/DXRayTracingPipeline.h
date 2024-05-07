#pragma once

#include "DXCommon.h"

struct DXRayTracingPipelineSettings
{
	// Root Signature Settings //
	CD3DX12_ROOT_PARAMETER* rayGenParameters = nullptr;
	unsigned int rayGenParameterCount = 0;

	CD3DX12_ROOT_PARAMETER* hitParameters = nullptr;
	unsigned int hitParameterCount = 0;

	CD3DX12_ROOT_PARAMETER* missParameters = nullptr;
	unsigned int missParameterCount = 0;
};

class DXRayTracingPipeline
{
public:
	DXRayTracingPipeline(DXRayTracingPipelineSettings settings);

private:
	void CreateRootSignature(ID3D12RootSignature** rootSignature,
		D3D12_ROOT_PARAMETER* parameterData, unsigned int parameterCount);

private:
	DXRayTracingPipelineSettings settings;

	ComPtr<IDxcBlob> rayGenLibrary;
	ComPtr<IDxcBlob> hitLibrary;
	ComPtr<IDxcBlob> missLibrary;

	ComPtr<ID3D12RootSignature> rayGenRootSignature;
	ComPtr<ID3D12RootSignature> hitRootSignature;
	ComPtr<ID3D12RootSignature> missRootSignature;
};