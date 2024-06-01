#pragma once

#include "DXCommon.h"
#include "DXAccess.h"
#include "DXUtilities.h"

#define ALIGN(_alignment, _val) (((_val + _alignment - 1) / _alignment) * _alignment)

#pragma region Acceleration Structure Helpers

inline void AllocateAccelerationStructureMemory(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs,
	ID3D12Resource** scratch, ID3D12Resource** result, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flag = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE)
{
	ComPtr<ID3D12Device5> device = DXAccess::GetDevice();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo); flag;

	D3D12_HEAP_PROPERTIES gpuHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	D3D12_RESOURCE_DESC resultDesc = CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	ThrowIfFailed(device->CreateCommittedResource(&gpuHeap, D3D12_HEAP_FLAG_NONE,
		&scratchDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(scratch)));

	ThrowIfFailed(device->CreateCommittedResource(&gpuHeap, D3D12_HEAP_FLAG_NONE,
		&resultDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(result)));
}

inline void BuildAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& inputs,
	ComPtr<ID3D12Resource> scratch, ComPtr<ID3D12Resource> result)
{
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC description = {};
	description.Inputs = inputs;
	description.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
	description.DestAccelerationStructureData = result->GetGPUVirtualAddress();

	DXCommands* commands = DXAccess::GetCommands(D3D12_COMMAND_LIST_TYPE_DIRECT);
	ComPtr<ID3D12GraphicsCommandList4> commandList = commands->GetGraphicsCommandList();

	commands->Flush();
	commands->ResetCommandList();

	commandList->BuildRaytracingAccelerationStructure(&description, 0, nullptr);

	commands->ExecuteCommandList();
	commands->Signal();
	commands->WaitForFenceValue();
}

#pragma endregion

#pragma region State Object Helpers

inline void AddLibrarySubobject(std::vector<D3D12_STATE_SUBOBJECT>& subobjects, unsigned int& index,
	ComPtr<IDxcBlob>& library, std::wstring* shaderSymbol)
{
	D3D12_EXPORT_DESC* exportDescription = new D3D12_EXPORT_DESC();
	exportDescription->Name = shaderSymbol->c_str();
	exportDescription->ExportToRename = nullptr;
	exportDescription->Flags = D3D12_EXPORT_FLAG_NONE;

	D3D12_DXIL_LIBRARY_DESC* libraryDescription = new D3D12_DXIL_LIBRARY_DESC();
	libraryDescription->DXILLibrary.pShaderBytecode = library->GetBufferPointer();
	libraryDescription->DXILLibrary.BytecodeLength = library->GetBufferSize();
	libraryDescription->NumExports = 1;
	libraryDescription->pExports = exportDescription;

	D3D12_STATE_SUBOBJECT libraryObject = {};
	libraryObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	libraryObject.pDesc = libraryDescription;

	subobjects[index] = libraryObject;
	index++;
}

inline void AddHitGroupSubobject(std::vector<D3D12_STATE_SUBOBJECT>& subobjects, unsigned int& index,
	std::wstring* hitGroupName, std::wstring* closestHitSymbol)
{
	D3D12_HIT_GROUP_DESC* hitGroupDescription = new D3D12_HIT_GROUP_DESC();
	hitGroupDescription->HitGroupExport = hitGroupName->c_str();
	hitGroupDescription->ClosestHitShaderImport = closestHitSymbol->c_str();

	D3D12_STATE_SUBOBJECT hitGroupObject = {};
	hitGroupObject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	hitGroupObject.pDesc = hitGroupDescription;

	subobjects[index] = hitGroupObject;
	index++;
}

inline void AddRootAssociationSubobject(std::vector<D3D12_STATE_SUBOBJECT>& subobjects, unsigned int& index,
	ComPtr<ID3D12RootSignature>& rootSignature, const WCHAR** rootExports, unsigned int exportCount)
{
	D3D12_STATE_SUBOBJECT rootSignatureSubobject = {};
	rootSignatureSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	rootSignatureSubobject.pDesc = rootSignature.GetAddressOf();

	subobjects[index] = rootSignatureSubobject;
	index++;

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION* rootAssociation = new D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION();
	rootAssociation->NumExports = exportCount;
	rootAssociation->pExports = rootExports;
	rootAssociation->pSubobjectToAssociate = &subobjects[index - 1];

	D3D12_STATE_SUBOBJECT rootAssociationSubobject = {};
	rootAssociationSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	rootAssociationSubobject.pDesc = rootAssociation;

	subobjects[index] = rootAssociationSubobject;
	index++;
}

inline void AddShaderPayloadSubobject(std::vector<D3D12_STATE_SUBOBJECT>& subobjects, unsigned int& index,
	unsigned int payLoadSize, unsigned int attributeSize, const WCHAR** shaderExports, unsigned int exportCount)
{
	D3D12_RAYTRACING_SHADER_CONFIG* shaderDesc = new D3D12_RAYTRACING_SHADER_CONFIG();
	shaderDesc->MaxPayloadSizeInBytes = payLoadSize;	// RGB and HitT
	shaderDesc->MaxAttributeSizeInBytes = attributeSize;

	D3D12_STATE_SUBOBJECT shaderConfigObject = {};
	shaderConfigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	shaderConfigObject.pDesc = shaderDesc;

	subobjects[index] = shaderConfigObject;
	index++;

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION* shaderPayloadAssociation = new D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION();
	shaderPayloadAssociation->pExports = shaderExports;
	shaderPayloadAssociation->NumExports = exportCount;
	shaderPayloadAssociation->pSubobjectToAssociate = &subobjects[(index - 1)];

	D3D12_STATE_SUBOBJECT shaderPayloadAssociationObject = {};
	shaderPayloadAssociationObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	shaderPayloadAssociationObject.pDesc = shaderPayloadAssociation;

	subobjects[index] = shaderPayloadAssociationObject;
	index++;
}

inline void AddDummyRootSignatureSubobjects(std::vector<D3D12_STATE_SUBOBJECT>& subobjects, unsigned int& index,
	ComPtr<ID3D12RootSignature>& globalRootSignature, ComPtr<ID3D12RootSignature>& localRootSignature)
{
	D3D12_STATE_SUBOBJECT globalRootSig;
	globalRootSig.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	globalRootSig.pDesc = globalRootSignature.GetAddressOf();

	subobjects[index] = globalRootSig;
	index++;

	// The pipeline construction always requires an empty local root signature
	D3D12_STATE_SUBOBJECT dummyLocalRootSig;
	dummyLocalRootSig.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	dummyLocalRootSig.pDesc = localRootSignature.GetAddressOf();

	subobjects[index] = dummyLocalRootSig;
	index++;
}
#pragma endregion