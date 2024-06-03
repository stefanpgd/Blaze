#include "Graphics/Mesh.h"
#include "Graphics/DXAccess.h"
#include "Graphics/DXUtilities.h"
#include "Graphics/DXRayTracingUtilities.h"
#include "Graphics/DXUploadBuffer.h"
#include "Graphics/Texture.h"
#include "Graphics/DXCommands.h"
#include "Framework/Mathematics.h"
#include <cassert>

Mesh::Mesh(tinygltf::Model& model, tinygltf::Primitive& primitive, glm::mat4& transform, bool isRayTracingGeometry)
{
	materialBuffer = new DXUploadBuffer(&material, sizeof(Material));

	LoadAttribute(model, primitive, "POSITION");
	LoadAttribute(model, primitive, "TEXCOORD_0");
	LoadAttribute(model, primitive, "NORMAL");

	LoadIndices(model, primitive);

	ApplyNodeTransform(transform);

	UploadBuffers();

	if(isRayTracingGeometry)
	{
		SetupGeometryDescription();
		BuildBLAS();
	}
}

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
		SetupGeometryDescription();
		BuildBLAS();
	}
}

void Mesh::UpdateMaterial()
{
	materialBuffer->UpdateData(&material);
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

ID3D12Resource* Mesh::GetIndexBuffer()
{
	return indexBuffer.Get();
}

D3D12_GPU_VIRTUAL_ADDRESS Mesh::GetMaterialGPUAddress()
{
	return materialBuffer->GetGPUVirtualAddress();
}

D3D12_RAYTRACING_GEOMETRY_DESC Mesh::GetGeometryDescription()
{
	return geometryDescription;
}

ID3D12Resource* Mesh::GetBLAS()
{
	return blasResult.Get();
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

void Mesh::SetupGeometryDescription()
{
	ComPtr<ID3D12Device5> device = DXAccess::GetDevice();

	geometryDescription.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	geometryDescription.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDescription.Triangles.Transform3x4 = 0;

	// Vertex Buffer //
	geometryDescription.Triangles.VertexBuffer.StartAddress = vertexBuffer->GetGPUVirtualAddress();
	geometryDescription.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
	geometryDescription.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geometryDescription.Triangles.VertexCount = verticesCount;

	// Index Buffer //
	geometryDescription.Triangles.IndexBuffer = indexBuffer->GetGPUVirtualAddress();
	geometryDescription.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
	geometryDescription.Triangles.IndexCount = indicesCount;
}

void Mesh::BuildBLAS()
{
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

	inputs.pGeometryDescs = &geometryDescription;
	inputs.NumDescs = 1;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE; // there are also other options like 'Fast Build'

	AllocateAccelerationStructureMemory(inputs, blasScratch.GetAddressOf(), blasResult.GetAddressOf());
	BuildAccelerationStructure(inputs, blasScratch, blasResult);
}

#pragma region TinyGLTF Loading
void Mesh::LoadAttribute(tinygltf::Model& model, tinygltf::Primitive& primitive, const std::string& attributeType)
{
	auto attribute = primitive.attributes.find(attributeType);

	// Check if within the primitives's attributes the type is present. For example 'Normals'
	// If not, stop here, the model isn't valid
	if(attribute == primitive.attributes.end())
	{
		std::string message = "Attribute Type: '" + attributeType + "' missing from model.";
		LOG(Log::MessageType::Debug, message);
		return;
	}

	// Accessor: Tells use which view we need, what type of data is in it, and the amount/count of data.
	// BufferView: Tells which buffer we need, and where we need to be in the buffer
	// Buffer: Binary data of our mesh
	tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at(attributeType)];
	tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
	tinygltf::Buffer& buffer = model.buffers[view.buffer];

	// Component: default type like float, int
	// Type: a structure made out of components, e.g VEC2 ( 2x float )
	unsigned int componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
	unsigned int objectSize = tinygltf::GetNumComponentsInType(accessor.type);
	unsigned int dataSize = componentSize * objectSize;

	// Accessor byteoffset: Offset to first element of type
	// BufferView byteoffset: Offset to get to this primitives buffer data in the overall buffer
	unsigned int bufferStart = accessor.byteOffset + view.byteOffset;

	// Stride: Distance in buffer till next elelemt occurs
	unsigned int stride = accessor.ByteStride(view);

	// In case it hasn't happened, resize the vertex buffer since we're 
	// going to directly memcpy the data into an already existing buffer
	if(vertices.size() < accessor.count)
	{
		vertices.resize(accessor.count);
	}

	for(int i = 0; i < accessor.count; i++)
	{
		Vertex& vertex = vertices[i];
		size_t bufferLocation = bufferStart + (i * stride);

		// TODO: Add 'offsetto' to this and use pointers to vertices instead of a reference
		if(attributeType == "POSITION")
		{
			memcpy(&vertex.Position, &buffer.data[bufferLocation], dataSize);
		}
		else if(attributeType == "TEXCOORD_0")
		{
			memcpy(&vertex.UVCoord, &buffer.data[bufferLocation], dataSize);
		}
		else if(attributeType == "NORMAL")
		{
			memcpy(&vertex.Normal, &buffer.data[bufferLocation], dataSize);
		}
	}
}

void Mesh::LoadIndices(tinygltf::Model& model, tinygltf::Primitive& primitive)
{
	tinygltf::Accessor& accessor = model.accessors[primitive.indices];
	tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
	tinygltf::Buffer& buffer = model.buffers[view.buffer];

	unsigned int componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
	unsigned int objectSize = tinygltf::GetNumComponentsInType(accessor.type);
	unsigned int dataSize = componentSize * objectSize;

	unsigned int bufferStart = accessor.byteOffset + view.byteOffset;
	unsigned int stride = accessor.ByteStride(view);

	for(int i = 0; i < accessor.count; i++)
	{
		size_t bufferLocation = bufferStart + (i * stride);

		if(componentSize == 2)
		{
			short index;
			memcpy(&index, &buffer.data[bufferLocation], dataSize);
			indices.push_back(index);
		}
		else if(componentSize == 4)
		{
			unsigned int index;
			memcpy(&index, &buffer.data[bufferLocation], dataSize);
			indices.push_back(index);
		}
	}
}

void Mesh::ApplyNodeTransform(const glm::mat4 transform)
{
	for(Vertex& vertex : vertices)
	{
		glm::vec4 vert = glm::vec4(vertex.Position.x, vertex.Position.y, vertex.Position.z, 1.0f);
		vertex.Position = transform * vert;

		glm::vec4 norm = glm::vec4(vertex.Normal.x, vertex.Normal.y, vertex.Normal.z, 0.0f);
		vertex.Normal = glm::normalize(transform * norm);
	}
}
#pragma endregion