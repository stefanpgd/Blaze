#pragma once

#include <vector>
#include <string>

#include "Graphics/DXCommon.h"
#include "Framework/Mathematics.h"

struct Vertex
{
	glm::vec3 Position;
	glm::vec2 UVCoord;
};

class Texture;

class Mesh
{
public:
	Mesh(Vertex* vertices, unsigned int vertexCount, unsigned int* indices, 
		unsigned int indexCount, bool isRayTracingGeometry = false);

	const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView();
	const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView();
	const unsigned int GetIndicesCount();

	D3D12_GPU_VIRTUAL_ADDRESS GetTLASAddress();

private:
	void UploadBuffers();

	void BuildRayTracingBLAS();
	void BuildRayTracingTLAS(); // TODO: Move to Model.cpp once we have it

public:
	std::string Name;

private:
	// Vertex & Index Data //
	ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	unsigned int verticesCount = 0;
	unsigned int indicesCount = 0;

	// Ray Tracing //
	bool isRayTracingGeometry;
	ComPtr<ID3D12Resource> blasScratch;
	ComPtr<ID3D12Resource> blasResult;

	ComPtr<ID3D12Resource> tlasInstanceDesc;
	ComPtr<ID3D12Resource> tlasScratch;
	ComPtr<ID3D12Resource> tlasResult;
};