#include "Graphics/DXShaderBindingTable.h"
#include "Graphics/DXRayTracingUtilities.h"
#include "Graphics/DXRayTracingPipeline.h"
#include "Graphics/DXTopLevelAS.h"

DXShaderBindingTable::DXShaderBindingTable(ID3D12StateObjectProperties* pipelineProperties) : pipelineProperties(pipelineProperties) { }

void DXShaderBindingTable::BuildShaderTable()
{
	CalculateShaderTableSizes();
	AllocateUploadResource(shaderTable, shaderTableSize);
	BindShaderTable();
	UpdateDispatchRayDescription();
}

void DXShaderBindingTable::ClearShaderTable()
{
	hitEntries.clear();
	shaderTable.Reset();
}

void DXShaderBindingTable::AddRayGenerationProgram(const std::wstring& identifier, const std::vector<void*>& inputs)
{
	rayGenEntry.identifier = identifier;
	rayGenEntry.inputs = inputs;
}

void DXShaderBindingTable::AddMissProgram(const std::wstring& identifier, const std::vector<void*>& inputs)
{
	missEntry.identifier = identifier;
	missEntry.inputs = inputs;
}

void DXShaderBindingTable::AddHitProgram(const std::wstring& identifier, const std::vector<void*>& inputs)
{
	ShaderTableEntry entry;
	entry.identifier = identifier;
	entry.inputs = inputs;

	hitEntries.push_back(entry);
}

const D3D12_DISPATCH_RAYS_DESC* DXShaderBindingTable::GetDispatchRayDescription()
{
	return &dispatchRayDescription;
}

void DXShaderBindingTable::BindShaderTable()
{
	uint8_t* pData;
	ThrowIfFailed(shaderTable->Map(0, nullptr, (void**)&pData));

	BindShaderRecord(rayGenEntry, pData);
	BindShaderRecord(missEntry, pData);

	for(ShaderTableEntry& hitEntry : hitEntries)
	{
		BindShaderRecord(hitEntry, pData);
	}

	shaderTable->Unmap(0, nullptr);
}

void DXShaderBindingTable::BindShaderRecord(ShaderTableEntry& entry, uint8_t*& ptr)
{
	memcpy(ptr, pipelineProperties->GetShaderIdentifier(entry.identifier.c_str()), shaderIdentifierSize);
	memcpy(ptr + shaderIdentifierSize, entry.inputs.data(), entry.inputs.size() * sizeof(UINT64));

	ptr += shaderRecordSize;
}

void DXShaderBindingTable::CalculateShaderTableSizes()
{
	// 1) Figure out the record with the most amount of entries, that input size
	// will be used to size the shaderRecord 
	size_t maxInputs = rayGenEntry.inputs.size();
	maxInputs = std::max(maxInputs, missEntry.inputs.size());

	// Loop over all hitEntries to check if any of them as more inputs than the miss or raygen entry.
	for(int i = 0; i < hitEntries.size(); i++)
	{
		maxInputs = std::max(maxInputs, hitEntries[i].inputs.size());
	}

	// 2) Based on the largest amount of inputs, determine the record size //
	shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	shaderRecordSize = shaderIdentifierSize;
	shaderRecordSize += maxInputs * sizeof(UINT64);
	shaderRecordSize = ALIGN(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, shaderRecordSize);

	// 3) Determine the shader table size //
	shaderTableSize = shaderRecordSize * (2 + hitEntries.size()); // RayGen, Miss and all hit entries 
	shaderTableSize = ALIGN(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT, shaderTableSize);
}

void DXShaderBindingTable::UpdateDispatchRayDescription()
{
	dispatchRayDescription.RayGenerationShaderRecord.StartAddress = shaderTable->GetGPUVirtualAddress();
	dispatchRayDescription.RayGenerationShaderRecord.SizeInBytes = shaderRecordSize;

	dispatchRayDescription.MissShaderTable.StartAddress = shaderTable->GetGPUVirtualAddress() + shaderRecordSize;
	dispatchRayDescription.MissShaderTable.SizeInBytes = shaderRecordSize;
	dispatchRayDescription.MissShaderTable.StrideInBytes = shaderRecordSize;

	dispatchRayDescription.HitGroupTable.StartAddress = shaderTable->GetGPUVirtualAddress() + (shaderRecordSize * 2);
	dispatchRayDescription.HitGroupTable.SizeInBytes = shaderRecordSize * hitEntries.size();
	dispatchRayDescription.HitGroupTable.StrideInBytes = shaderRecordSize;

	dispatchRayDescription.Width = DXAccess::GetWindow()->GetWindowWidth();
	dispatchRayDescription.Height = DXAccess::GetWindow()->GetWindowHeight();
	dispatchRayDescription.Depth = 1;
}