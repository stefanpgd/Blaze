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

void DXShaderBindingTable::SetRayGenerationProgram(const std::wstring& identifier, const std::vector<void*>& inputs)
{
	rayGenEntry.identifier = identifier;
	rayGenEntry.inputs = inputs;
}

void DXShaderBindingTable::SetMissProgram(const std::wstring& identifier, const std::vector<void*>& inputs)
{
	missEntry.identifier = identifier;
	missEntry.inputs = inputs;
}

void DXShaderBindingTable::SetHitProgram(const std::wstring& identifier, const std::vector<void*>& inputs)
{
	hitEntry.identifier = identifier;
	hitEntry.inputs = inputs;
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
	BindShaderRecord(hitEntry, pData);

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
	size_t maxInputs = 0;
	maxInputs = std::max(rayGenEntry.inputs.size(), missEntry.inputs.size());
	maxInputs = std::max(maxInputs, hitEntry.inputs.size());

	// TODO: Figure out the largest 'record' and based on that adjust all other record sizes 
	shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	shaderRecordSize = shaderIdentifierSize;
	shaderRecordSize += maxInputs * sizeof(UINT64); 

	// Aligns record to be 64-byte, ensuring alignment //
	shaderRecordSize = ALIGN(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, shaderRecordSize);

	shaderTableSize = shaderRecordSize * 3; // 3 shader entries //
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
	dispatchRayDescription.HitGroupTable.SizeInBytes = shaderRecordSize;
	dispatchRayDescription.HitGroupTable.StrideInBytes = shaderRecordSize;

	dispatchRayDescription.Width = DXAccess::GetWindow()->GetWindowWidth();
	dispatchRayDescription.Height = DXAccess::GetWindow()->GetWindowHeight();
	dispatchRayDescription.Depth = 1;
}