#include "Graphics/Mesh.h"
#include "Graphics/DXAccess.h"
#include "Graphics/DXUtilities.h"
#include "Graphics/Texture.h"
#include "Graphics/DXCommands.h"
#include <cassert>

Mesh::Mesh(Vertex* verts, unsigned int vertexCount, unsigned int* indi, 
	unsigned int indexCount, bool isRayTracingGeometry) : isRayTracingGeometry(isRayTracingGeometry)
{
	for(int i = 0; i < vertexCount; i++)
	{
		vertices.push_back(verts[i]);
	}

	for(int i = 0; i < indexCount; i++)
	{
		indices.push_back(indi[i]);
	}

	UploadBuffers();

	if(isRayTracingGeometry)
	{
		BuildRayTracingBLAS();
	}
}

const D3D12_VERTEX_BUFFER_VIEW& Mesh::GetVertexBufferView()
{
	return vertexBufferView;
}

const D3D12_INDEX_BUFFER_VIEW& Mesh::GetIndexBufferView()
{
	return indexBufferView;
}

const unsigned int Mesh::GetIndicesCount()
{
	return indicesCount;
}

void Mesh::UploadBuffers()
{
	DXCommands* copyCommands = DXAccess::GetCommands(D3D12_COMMAND_LIST_TYPE_COPY);
	ComPtr<ID3D12GraphicsCommandList4> copyCommandList = copyCommands->GetGraphicsCommandList();
	copyCommands->ResetCommandList();

	// 1. Record commands to upload vertex & index buffers //
	ComPtr<ID3D12Resource> intermediateVertexBuffer;
	UpdateBufferResource(copyCommandList, &vertexBuffer, &intermediateVertexBuffer, vertices.size(),
		sizeof(Vertex), vertices.data(), D3D12_RESOURCE_FLAG_NONE);

	ComPtr<ID3D12Resource> intermediateIndexBuffer;
	UpdateBufferResource(copyCommandList, &indexBuffer, &intermediateIndexBuffer, indices.size(),
		sizeof(unsigned int), indices.data(), D3D12_RESOURCE_FLAG_NONE);

	// 2. Retrieve info about from the buffers to create Views  // 
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = vertices.size() * sizeof(Vertex);
	vertexBufferView.StrideInBytes = sizeof(Vertex);

	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes = indices.size() * sizeof(unsigned int);
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	// 3.Execute the copying on the command queue & wait until it's done // 
	copyCommands->ExecuteCommandList(DXAccess::GetCurrentBackBufferIndex());
	copyCommands->Signal();
	copyCommands->WaitForFenceValue(DXAccess::GetCurrentBackBufferIndex());

	// 4. Clear CPU data // 
	verticesCount = vertices.size();
	indicesCount = indices.size();

	vertices.clear();
	indices.clear();
}

// TODO: Check based on material info if its opaque or transparent..
void Mesh::BuildRayTracingBLAS()
{
	ComPtr<ID3D12Device5> device = DXAccess::GetDevice();

	// 1. Define the Geometry that will be part of the BLAS //
	D3D12_RAYTRACING_GEOMETRY_DESC geometry;
	geometry.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE; 
	geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometry.Triangles.Transform3x4 = 0;

	// Vertex Buffer //
	geometry.Triangles.VertexBuffer.StartAddress = vertexBuffer->GetGPUVirtualAddress();
	geometry.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
	geometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geometry.Triangles.VertexCount = verticesCount;

	// Index Buffer //
	geometry.Triangles.IndexBuffer = indexBuffer->GetGPUVirtualAddress();
	geometry.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
	geometry.Triangles.IndexCount = indicesCount;

	// 2. Determine the memory needed to build the structure, which includes setting up info about the BLS //
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

	inputs.pGeometryDescs = &geometry;
	inputs.NumDescs = 1;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE; // there are also other options like 'Fast Build'

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO buildInfo = {};
	device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &buildInfo);

	// 3. Allocate memory for Scratch & Final BLAS //
	D3D12_HEAP_PROPERTIES gpuHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(buildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	D3D12_RESOURCE_DESC resultDesc = CD3DX12_RESOURCE_DESC::Buffer(buildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	ThrowIfFailed(device->CreateCommittedResource(&gpuHeap, D3D12_HEAP_FLAG_NONE,
		&scratchDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&blasScratch)));

	ThrowIfFailed(device->CreateCommittedResource(&gpuHeap, D3D12_HEAP_FLAG_NONE,
		&resultDesc, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&blasResult)));

	// 4. Build the Acceleration Structure //
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC BLASdescription = {};
	BLASdescription.Inputs = inputs;
	BLASdescription.ScratchAccelerationStructureData = blasScratch->GetGPUVirtualAddress();
	BLASdescription.DestAccelerationStructureData = blasResult->GetGPUVirtualAddress();

	DXCommands* commands = DXAccess::GetCommands(D3D12_COMMAND_LIST_TYPE_DIRECT);
	ComPtr<ID3D12GraphicsCommandList4> commandList = commands->GetGraphicsCommandList();

	commands->Flush();
	commands->ResetCommandList();

	commandList->BuildRaytracingAccelerationStructure(&BLASdescription, 0, nullptr);

	commands->ExecuteCommandList();
	commands->Signal();
	commands->WaitForFenceValue();
}