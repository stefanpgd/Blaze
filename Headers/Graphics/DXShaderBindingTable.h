#pragma once

#include "DXCommon.h"
#include <vector>

struct DXRayTracingPipelineSettings;

struct ShaderTableEntry
{
	std::wstring identifier;
	std::vector<void*> inputs;
};

class DXShaderBindingTable
{
public:
	DXShaderBindingTable(ID3D12StateObjectProperties* pipelineProperties);

	void BuildShaderTable();

	void SetRayGenerationProgram(const std::wstring& identifier, const std::vector<void*>& inputs);
	void SetMissProgram(const std::wstring& identifier, const std::vector<void*>& inputs);
	void SetHitProgram(const std::wstring& identifier, const std::vector<void*>& inputs);

	const D3D12_DISPATCH_RAYS_DESC* GetDispatchRayDescription();

private:
	void BindShaderTable();
	void BindShaderRecord(ShaderTableEntry& entry, uint8_t*& ptr);
	void CalculateShaderTableSizes();
	void UpdateDispatchRayDescription();

private:
	// TODO: It can be that we need multiple programs, if that ends up being the case
	// make sure to change 'Set' to 'Add' and instead make the individual entries vectors
	ShaderTableEntry rayGenEntry;
	ShaderTableEntry missEntry;
	ShaderTableEntry hitEntry;

	ComPtr<ID3D12Resource> shaderTable;
	D3D12_DISPATCH_RAYS_DESC dispatchRayDescription;
	ID3D12StateObjectProperties* pipelineProperties;

	unsigned int shaderTableSize = 0;		// Size of all shader records 
	unsigned int shaderRecordSize = 0;		// Size of the entire record of a shader 
	unsigned int shaderIdentifierSize = 0;	// Size of the 'shader'
};