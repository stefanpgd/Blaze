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

	void CompileShaderLibrary(IDxcBlob** shader, std::wstring shaderName);

private:
	DXRayTracingPipelineSettings settings;

	IDxcCompiler* compiler;
	IDxcLibrary* library;
	ComPtr<IDxcIncludeHandler> dxcIncludeHandler;

	ComPtr<IDxcBlob> rayGenLibrary;
	ComPtr<IDxcBlob> hitLibrary;
	ComPtr<IDxcBlob> missLibrary;

	ComPtr<ID3D12RootSignature> rayGenRootSignature;
	ComPtr<ID3D12RootSignature> hitRootSignature;
	ComPtr<ID3D12RootSignature> missRootSignature;

	ComPtr<ID3D12StateObject> pipeline;
	ComPtr<ID3D12StateObjectProperties> pipelineProprties;
};