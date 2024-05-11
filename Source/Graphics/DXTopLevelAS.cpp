#include "Graphics/DXTopLevelAS.h"
#include "Graphics/DXAccess.h"
#include "Graphics/Mesh.h"
#include "Graphics/DXUtilities.h"
#include "Graphics/DXRayTracingUtilities.h"
#include "Framework/Mathematics.h"

// TODO: Once we've a bit more control, such as an editor
// we want to be able to pass "models", which already contain a transform we can link against
// Then once we know the editor updates some transform, we can simply recompile
// the TLAS
DXTopLevelAS::DXTopLevelAS(Mesh* mesh) : mesh(mesh)
{
	BuildTLAS();
}

void DXTopLevelAS::BuildTLAS()
{
	ComPtr<ID3D12Device5> device = DXAccess::GetDevice();

	D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
	instanceDesc.InstanceID = 0;
	instanceDesc.InstanceContributionToHitGroupIndex = 0;
	instanceDesc.InstanceMask = 0xFF;
	instanceDesc.AccelerationStructure = mesh->GetBLAS()->GetGPUVirtualAddress();
	instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;

	glm::mat4 test = glm::mat4(1.0f);
	test = glm::translate(test, glm::vec3(0.0f, 1.0f, 0.0f));
	test = glm::scale(test, glm::vec3(2.0f, 0.5f, 1.0f));

	for(int x = 0; x < 3; x++)
	{
		for(int y = 0; y < 4; y++)
		{
			instanceDesc.Transform[x][y] = test[y][x];
		}
	}

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

D3D12_GPU_VIRTUAL_ADDRESS DXTopLevelAS::GetTLASAddress()
{
	return tlasResult->GetGPUVirtualAddress();
}