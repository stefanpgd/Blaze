#pragma once

#include <string>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXRootSignature;

class DXComputePipeline
{
public:
	DXComputePipeline(DXRootSignature* rootSignature, std::string computePath);

	ComPtr<ID3D12PipelineState> Get();
	ID3D12PipelineState* GetAddress();

private:
	void CompileShaders(std::string computePath);
	void CreatePipelineState();

private:
	DXRootSignature* rootSignature;

	ComPtr<ID3D12PipelineState> pipeline;
	ComPtr<ID3DBlob> computeShaderBlob;
};

