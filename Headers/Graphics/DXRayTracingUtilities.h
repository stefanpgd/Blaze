#pragma once

#include "DXCommon.h"
#include "DXAccess.h"

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