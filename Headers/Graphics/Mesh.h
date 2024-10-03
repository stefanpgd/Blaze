#pragma once

#include <vector>
#include <string>

#include "Graphics/DXCommon.h"
#include "Framework/Mathematics.h"
#include "Material.h"

#include <tiny_gltf.h>

class Texture;
class DXUploadBuffer;

struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec3 Tangent;
	glm::vec2 TextureCoord0;
};

class Mesh
{
public:
	Mesh(Vertex* vertices, unsigned int vertexCount, unsigned int* indices, 
		unsigned int indexCount, bool isRayTracingGeometry = false);

	Mesh(tinygltf::Model& model, tinygltf::Primitive& primitive,
		glm::mat4& transform, bool isRayTracingGeometry = false);

	void UpdateMaterial();

	const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView();
	const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView();
	const unsigned int GetIndicesCount();

	ID3D12Resource* GetVertexBuffer();
	ID3D12Resource* GetIndexBuffer();
	D3D12_GPU_VIRTUAL_ADDRESS GetMaterialGPUAddress();

	// Ray Tracing //
	D3D12_RAYTRACING_GEOMETRY_DESC GetGeometryDescription();
	ID3D12Resource* GetBLAS();

	// TEMP //
	Texture* diffuseTexture;
	Texture* normalTexture;
	Texture* ORMTexture;

private:
	void GenerateTangents();
	void UploadGeometryBuffers();
	void SetupGeometryDescription();
	void BuildBLAS();

public:
	std::string Name;
	Material material;

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

	// Texture & Material Data //
	DXUploadBuffer* materialBuffer;

	// Ray Tracing //
	bool isRayTracingGeometry;
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDescription;
	ComPtr<ID3D12Resource> blasScratch;
	ComPtr<ID3D12Resource> blasResult;
};