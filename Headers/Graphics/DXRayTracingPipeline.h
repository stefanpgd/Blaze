#pragma once

#include "DXCommon.h"

struct DXRayTracingPipelineSettings
{

};

class DXRayTracingPipeline
{
public:
	DXRayTracingPipeline();

private:
	void CreateRootSignature(ID3D12RootSignature** rootSignature,
		D3D12_ROOT_PARAMETER* parameterData, unsigned int parameterCount);

private:
	ComPtr<IDxcBlob> rayGenLibrary;
	ComPtr<IDxcBlob> hitLibrary;
	ComPtr<IDxcBlob> missLibrary;

	ComPtr<ID3D12RootSignature> rayGenRootSignature;
	ComPtr<ID3D12RootSignature> hitRootSignature;
	ComPtr<ID3D12RootSignature> missRootSignature;
};