#include "Graphics/DXTopLevelAS.h"
#include "Graphics/DXAccess.h"
#include "Graphics/DXUtilities.h"
#include "Graphics/DXRayTracingUtilities.h"
#include "Graphics/DXDescriptorHeap.h"
#include "Graphics/Model.h"
#include "Graphics/Mesh.h"

#include "Framework/Scene.h"
#include "Framework/Mathematics.h"

DXTopLevelAS::DXTopLevelAS(Scene* scene) : activeScene(scene)
{
	BuildTLAS();
}

void DXTopLevelAS::RebuildTLAS()
{
	// 1) Make sure the current TLAS memory is cleared //
	DXCommands* commands = DXAccess::GetCommands(D3D12_COMMAND_LIST_TYPE_DIRECT);
	commands->Flush();

	tlasScratch.Reset();
	tlasResult.Reset();

	// 2) Rebuild TLAS //
	BuildTLAS();
}

void DXTopLevelAS::BuildTLAS()
{
	// 1) Figure out how many instances we wanna have 
	int instanceCount = 0;
	
	auto models = activeScene->GetModels();
	for(Model* model : models)
	{
		instanceCount += model->GetMeshCount();
	}

	ComPtr<ID3D12Device5> device = DXAccess::GetDevice();
	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instances(instanceCount);

	// 2) Iterate over meshes, create a instance description for each 
	int instanceIndex = 0;
	for(Model* model : models)
	{
		glm::mat4 transform = model->transform.GetModelMatrix();

		const std::vector<Mesh*>& meshes = model->GetMeshes();
		for(Mesh* mesh : meshes)
		{
			D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
			instanceDesc.InstanceID = instanceIndex;
			instanceDesc.InstanceContributionToHitGroupIndex = instanceIndex;
			instanceDesc.InstanceMask = 0xFF;
			instanceDesc.AccelerationStructure = mesh->GetBLAS()->GetGPUVirtualAddress();

			for(int x = 0; x < 3; x++)
			{
				for(int y = 0; y < 4; y++)
				{
					instanceDesc.Transform[x][y] = transform[y][x];
				}
			}

			instances[instanceIndex] = instanceDesc;
			instanceIndex++;
		}
	}

	// 3) Allocate a buffer to map all the instance data to & Build the TLAS 
	unsigned int dataSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instances.size();
	AllocateAndMapResource(tlasInstanceDesc, instances.data(), dataSize);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.InstanceDescs = tlasInstanceDesc->GetGPUVirtualAddress();
	inputs.NumDescs = instances.size();
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	AllocateAccelerationStructureMemory(inputs, tlasScratch.GetAddressOf(), tlasResult.GetAddressOf());
	BuildAccelerationStructure(inputs, tlasScratch, tlasResult);
}

void DXTopLevelAS::SetScene(Scene* scene)
{
	activeScene = scene;
	RebuildTLAS();
}

D3D12_GPU_VIRTUAL_ADDRESS DXTopLevelAS::GetGPUVirtualAddress()
{
	return tlasResult->GetGPUVirtualAddress();
}