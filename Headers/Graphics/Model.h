#pragma once

#include <string>
#include <vector>
#include <tiny_gltf.h>

#include "Graphics/Transform.h"
#include <wrl.h>
#include <d3d12.h>
using namespace Microsoft::WRL;

class Mesh;
struct Vertex;

class Model
{
public:
	Model(const std::string& filePath, bool isRayTracingGeometry = false);

	Model(Vertex* vertices, unsigned int vertexCount, unsigned int* indices,
		unsigned int indexCount, bool isRayTracingGeometry = false);

	Mesh* GetMesh(int index);
	const std::vector<Mesh*>& GetMeshes();

	ID3D12Resource* GetBLAS();

private:
	void TraverseRootNodes(tinygltf::Model& model);
	void TraverseChildNodes(tinygltf::Model& model, tinygltf::Node& node, const glm::mat4& parentMatrix);

	glm::mat4 GetTransformFromNode(tinygltf::Node& node);
	
	void BuildBLAS();

public:
	Transform transform;
	std::string Name;

private:
	std::vector<Mesh*> meshes;

	// Ray Tracing //
	bool isRayTracingGeometry;
	ComPtr<ID3D12Resource> blasScratch;
	ComPtr<ID3D12Resource> blasResult;
};