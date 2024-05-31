#pragma once

#include "DXCommon.h"

class DXTopLevelAS;
class DXDescriptorHeap;
class Texture;

struct DXRayTracingPipelineSettings
{
	// Root Signature Settings //
	CD3DX12_ROOT_PARAMETER* rayGenParameters = nullptr;
	unsigned int rayGenParameterCount = 0;

	CD3DX12_ROOT_PARAMETER* hitParameters = nullptr;
	unsigned int hitParameterCount = 0;

	CD3DX12_ROOT_PARAMETER* missParameters = nullptr;
	unsigned int missParameterCount = 0;

	// Pipeline Settings //
	float payLoadSize = sizeof(float) * 4;
	float attributeSize = sizeof(float) * 2;
	float maxRayRecursionDepth = 1;
};

class DXRayTracingPipeline
{
public:
	DXRayTracingPipeline(DXRayTracingPipelineSettings settings);

	ID3D12StateObject* GetPipelineState();
	ID3D12StateObjectProperties* GetPipelineProperties();

private:
	void CreatePipeline();

	void CreateRootSignature(ComPtr<ID3D12RootSignature>& rootSignature,
		D3D12_ROOT_PARAMETER* parameterData, unsigned int parameterCount, bool isLocal);
	
	void CompileShaderLibrary(ComPtr<IDxcBlob>& shaderLibrary, std::wstring shaderName);

private:
	DXRayTracingPipelineSettings settings;

	ComPtr<ID3D12StateObject> pipeline;
	ComPtr<ID3D12StateObjectProperties> pipelineProperties;
	ComPtr<ID3D12Resource> shaderTable;

	IDxcCompiler* compiler;
	IDxcLibrary* library;
	IDxcIncludeHandler* dxcIncludeHandler;

	ComPtr<IDxcBlob> rayGenLibrary;
	ComPtr<IDxcBlob> hitLibrary;
	ComPtr<IDxcBlob> missLibrary;

	ComPtr<ID3D12RootSignature> rayGenRootSignature;
	ComPtr<ID3D12RootSignature> hitRootSignature;
	ComPtr<ID3D12RootSignature> missRootSignature;

	ComPtr<ID3D12RootSignature> localDummyRootSignature;
	ComPtr<ID3D12RootSignature> globalDummyRootSignature;

};