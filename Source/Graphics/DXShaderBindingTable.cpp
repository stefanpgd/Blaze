#include "Graphics/DXShaderBindingTable.h"
#include "Graphics/DXRayTracingUtilities.h"
#include "Graphics/DXRayTracingPipeline.h"
#include "Graphics/DXTopLevelAS.h"

DXShaderBindingTable::DXShaderBindingTable(ID3D12StateObjectProperties* pipelineProperties, DXRayTracingPipelineSettings& settings)
	: pipelineProperties(pipelineProperties), settings(settings)
{
	BuildShaderTable();
}

const D3D12_DISPATCH_RAYS_DESC* DXShaderBindingTable::GetDispatchRayDescription()
{
	return &dispatchRayDescription;
}

void DXShaderBindingTable::BuildShaderTable()
{
	CalculateShaderTableSizes();
	AllocateUploadResource(shaderTable, shaderTableSize);
	BindShaderTable();
	UpdateDispatchRayDescription();
}

void DXShaderBindingTable::CalculateShaderTableSizes()
{
	// TODO: Figure out the largest 'record' and based on that adjust all other record sizes 
	shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	shaderRecordSize = shaderIdentifierSize;
	shaderRecordSize += 8; // UAV-SRV Descriptor Table //
	shaderRecordSize += 8; // UAV-SRV Descriptor Table - 2 //

	// Aligns record to be 64-byte, ensuring alignment //
	shaderRecordSize = ALIGN(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, shaderRecordSize);

	shaderTableSize = shaderRecordSize * 3; // 3 shader entries //
	shaderTableSize = ALIGN(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT, shaderTableSize);
}

void DXShaderBindingTable::BindShaderTable()
{
	uint8_t* pData;
	ThrowIfFailed(shaderTable->Map(0, nullptr, (void**)&pData));

	// Shader Record 0 - Ray Generation program and local root parameter data 
	memcpy(pData, pipelineProperties->GetShaderIdentifier(L"RayGen"), shaderIdentifierSize);

	// Set the root parameter data, Point to start of descriptor heap //
	*reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(pData + shaderIdentifierSize) = settings.uavSrvHeap->GetGPUHandleAt(0);

	// Shader Record 1 - Miss Record 
	pData += shaderRecordSize;
	memcpy(pData, pipelineProperties->GetShaderIdentifier(L"Miss"), shaderIdentifierSize);
	*reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(pData + shaderIdentifierSize) = settings.uavSrvHeap->GetGPUHandleAt(4);

	// Shader Record 2 - HitGroup Record 
	pData += shaderRecordSize;
	memcpy(pData, pipelineProperties->GetShaderIdentifier(L"HitGroup"), shaderIdentifierSize);
	*reinterpret_cast<UINT64*>(pData + shaderIdentifierSize) = settings.vertexBuffer->GetGPUVirtualAddress();
	*reinterpret_cast<UINT64*>(pData + shaderIdentifierSize + 8) = settings.indexBuffer->GetGPUVirtualAddress();
	*reinterpret_cast<UINT64*>(pData + shaderIdentifierSize + 16) = settings.TLAS->GetTLASAddress();

	shaderTable->Unmap(0, nullptr);
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