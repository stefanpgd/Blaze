#pragma once

#include "DXCommon.h"

struct DXRayTracingPipelineSettings;

class DXShaderBindingTable
{
public:
	DXShaderBindingTable(ID3D12StateObjectProperties* pipelineProperties, DXRayTracingPipelineSettings& settings);

	const D3D12_DISPATCH_RAYS_DESC* GetDispatchRayDescription();

private:
	void BuildShaderTable();

	void CalculateShaderTableSizes();
	void BindShaderTable();
	void UpdateDispatchRayDescription();

private:
	// TODO: remove placeholder when applicable //
	ID3D12StateObjectProperties* pipelineProperties;
	DXRayTracingPipelineSettings& settings;

	ComPtr<ID3D12Resource> shaderTable;
	D3D12_DISPATCH_RAYS_DESC dispatchRayDescription;

	unsigned int shaderTableSize = 0;		// Size of all shader records 
	unsigned int shaderRecordSize = 0;		// Size of the entire record of a shader 
	unsigned int shaderIdentifierSize = 0;	// Size of the 'shader'
};