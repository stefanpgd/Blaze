#pragma once

#include <tiny_gltf.h>
#include <string>
#include <vector>
#include "Graphics/Mesh.h"
#include "Utilities/Logger.h"

enum glTFTextureType
{
	BaseColor,
	Normal,
	MetallicRoughness,
	Occlusion, 
};

/// <summary>
/// Able to load in a specific 'Attribute' defined by glTF. For example with 'POSITION' all
/// position data can be loaded into a given buffer of vertices. 
/// </summary>
inline void glTFLoadVertexAttribute(std::vector<Vertex>& vertices, const std::string& attributeType, 
	tinygltf::Model& model, tinygltf::Primitive& primitive)
{
	auto attribute = primitive.attributes.find(attributeType);

	// Check if within the primitives's attributes the type is present. For example 'Normals'
	// If not, return here, since there is no data to load in. 
	if(attribute == primitive.attributes.end())
	{
		std::string message = "Attribute Type: '" + attributeType + "' missing from this mesh.";
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

	if(attributeType == "TANGENT")
	{
		// Tangent is something stored as float4, but in this framework we 
		// only care about the first 3 components. 
		dataSize = sizeof(float) * 3;
	}

	// Accessor byteoffset: Offset to first element of type
	// BufferView byteoffset: Offset to get to this primitives buffer data in the overall buffer
	unsigned int bufferStart = accessor.byteOffset + view.byteOffset;

	// Stride: Distance in buffer till next element occurs
	unsigned int stride = accessor.ByteStride(view);

	// In case it hasn't happened, resize the vertex buffer since we're 
	// going to directly memcpy the data into an already existing buffer
	if(vertices.size() < accessor.count)
	{
		vertices.resize(accessor.count);
	}

	// Copy data directly into vertex attribute from the glTF buffer(s)
	for(int i = 0; i < accessor.count; i++)
	{
		Vertex& vertex = vertices[i];
		size_t bufferLocation = bufferStart + (i * stride);

		// TODO: Could be an option to just grab the pointer and pass an offset, saving the if-else..
		if(attributeType == "POSITION")
		{
			memcpy(&vertex.Position, &buffer.data[bufferLocation], dataSize);
		}
		else if(attributeType == "NORMAL")
		{
			memcpy(&vertex.Normal, &buffer.data[bufferLocation], dataSize);
		}
		else if(attributeType == "TANGENT")
		{
			memcpy(&vertex.Tangent, &buffer.data[bufferLocation], sizeof(float) * 3);
		}
		else if(attributeType == "TEXCOORD_0")
		{
			memcpy(&vertex.TextureCoord0, &buffer.data[bufferLocation], dataSize);
		}
	}
}

void glTFLoadIndices(std::vector<unsigned int>& indices,
	tinygltf::Model& model, tinygltf::Primitive& primitive)
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

/// <summary>
/// Once a model is loaded in, it requires to be transformed with the matrix it was stored with.\
/// This goes over all the model data and transforms the relevant components.
/// </summary>
inline void glTFApplyNodeTransform(std::vector<Vertex>& vertices, const glm::mat4& transform)
{
	for(Vertex& vertex : vertices)
	{
		glm::vec4 vert = glm::vec4(vertex.Position.x, vertex.Position.y, vertex.Position.z, 1.0f);
		vertex.Position = transform * vert;

		glm::vec4 norm = glm::vec4(vertex.Normal.x, vertex.Normal.y, vertex.Normal.z, 0.0f);
		vertex.Normal = glm::normalize(transform * norm);

		glm::vec4 tang = glm::vec4(vertex.Tangent.x, vertex.Tangent.y, vertex.Tangent.z, 0.0f);
		vertex.Tangent = glm::normalize(transform * tang);
	}
}

/// <summary>
/// Able to load-in a texture with the glTF data. It checks if the texture type is present.
/// If so, it will load it in, otherwise return the framework's default texture.
/// It will also return a boolean indicating if loading the texture was succesful.
/// </summary>
inline bool glTFLoadTextureByType(Texture** texture, glTFTextureType type, tinygltf::Model& model, tinygltf::Primitive& primitive)
{
	// If it doesn't contain any materials, no textures to load
	if(model.materials.size() > 0)
	{
		tinygltf::Material& mat = model.materials[primitive.material];
		int textureIndex = -1;

		switch(type)
		{
		case glTFTextureType::BaseColor:
			textureIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
			break;
		case glTFTextureType::Normal:
			textureIndex = mat.normalTexture.index;
			break;
		case glTFTextureType::MetallicRoughness:
			textureIndex = mat.pbrMetallicRoughness.metallicRoughnessTexture.index;
			break;
		case glTFTextureType::Occlusion:
			textureIndex = mat.occlusionTexture.index;
			break;
		}

		if(textureIndex != -1)
		{
			tinygltf::Image& image = model.images[textureIndex];
			*texture = new Texture(image.image.data(), image.width, image.height);
			return true;
		}
	}

	// TODO: replace hardcoded path with Texture Database 'default'
	*texture = new Texture("Assets/Textures/missing.png");
	return false;
}