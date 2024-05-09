#include "Graphics/Mesh.h"
#include "Graphics/DXAccess.h"
#include "Graphics/DXUtilities.h"
#include "Graphics/DXRayTracingUtilities.h"
#include "Graphics/Texture.h"
#include "Graphics/DXCommands.h"
#include "Framework/Mathematics.h"
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
		BuildRayTracingTLAS();
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

ID3D12Resource* Mesh::GetVertexBuffer()
{
	return vertexBuffer.Get();
}

D3D12_GPU_VIRTUAL_ADDRESS Mesh::GetTLASAddress()
{
	return tlasResult->GetGPUVirtualAddress();
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

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

	inputs.pGeometryDescs = &geometry;
	inputs.NumDescs = 1;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE; // there are also other options like 'Fast Build'

	AllocateAccelerationStructureMemory(inputs, blasScratch.GetAddressOf(), blasResult.GetAddressOf());
	BuildAccelerationStructure(inputs, blasScratch, blasResult);
}

void Mesh::BuildRayTracingTLAS()
{
	ComPtr<ID3D12Device5> device = DXAccess::GetDevice();

	// Ty, Adam Marrs - https://github.com/acmarrs/IntroToDXR/blob/master/src/Graphics.cpp
	D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
	instanceDesc.InstanceID = 0;
	instanceDesc.InstanceContributionToHitGroupIndex = 0;
	instanceDesc.InstanceMask = 0xFF;
	instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
	instanceDesc.AccelerationStructure = blasResult->GetGPUVirtualAddress();

	AllocateAndMapResource(tlasInstanceDesc, &instanceDesc, sizeof(instanceDesc));

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.InstanceDescs = tlasInstanceDesc->GetGPUVirtualAddress();
	inputs.NumDescs = 1;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	
	AllocateAccelerationStructureMemory(inputs, tlasScratch.GetAddressOf(), tlasResult.GetAddressOf());
	BuildAccelerationStructure(inputs, tlasScratch, tlasResult);
}