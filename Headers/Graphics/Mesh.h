#pragma once

#include <vector>
#include <string>
#include <d3d12.h>
#include <d3dx12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

#include "Framework/Mathematics.h"

struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec3 Tangent;
	glm::vec3 Color;
	glm::vec2 TexCoord;
};

class Texture;

class Mesh
{
public:
	Mesh(Vertex* vertices, unsigned int vertexCount, unsigned int* indices, unsigned int indexCount);

	const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView();
	const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView();
	const CD3DX12_GPU_DESCRIPTOR_HANDLE GetMaterialView();
	const unsigned int GetIndicesCount();

	bool HasTextures();
	unsigned int GetTextureID();

private:
	void UploadBuffers();

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
	unsigned int indicesCount = 0;


	bool hasTextures = false;

	int materialCBVIndex = -1;
	ComPtr<ID3D12Resource> materialBuffer;
};