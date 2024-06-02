#include "Graphics/RenderStages/RayTraceStage.h"
#include "Graphics/DXRayTracingPipeline.h"
#include "Graphics/DXShaderBindingTable.h"
#include "Graphics/DXTopLevelAS.h"
#include "Framework/Scene.h"

#include "Graphics/DXUtilities.h"
#include "Graphics/DXUploadBuffer.h"
#include "Graphics/Model.h"
#include "Graphics/Mesh.h"
#include "Graphics/Texture.h"
#include "Graphics/EnvironmentMap.h"
#include <vector>

RayTraceStage::RayTraceStage(Scene* scene) : activeScene(scene)
{	
	CreateShaderResources();
	CreateShaderDescriptors();

	TLAS = new DXTopLevelAS(scene);
	InitializePipeline();
	InitializeShaderBindingTable();
}

void RayTraceStage::Update(float deltaTime)
{
	settings.frameCount++;
	settings.time += deltaTime;

	// The RayTraceStage has a buffer of relevant information about the application
	// Things like time, frame count, and some settings like that it needs to clear the screen.
	// Based on the information of the scene & app, we adjust the pipeline accordingly 
	if(activeScene->HasGeometryMoved)
	{
		// TODO: Even though this works, we need to find a proper place to fit this in, for example
		// how resizing is handled within Nova 
		TLAS->RebuildTLAS();

		CreateShaderDescriptors();
		InitializePipeline();
		InitializeShaderBindingTable();

		activeScene->HasGeometryMoved = false;
		settings.frameCount = 0;
		settings.clearBuffers = true;
	}
	else
	{
		settings.clearBuffers = false;
	}

	settingsBuffer->UpdateData(&settings);
}

void RayTraceStage::RecordStage(ComPtr<ID3D12GraphicsCommandList4> commandList)
{
	ComPtr<ID3D12Resource> renderTargetBuffer = DXAccess::GetWindow()->GetCurrentScreenBuffer();
	ID3D12Resource* const output = outputBuffer->GetAddress();

	// 1) Prepare render buffer & Run the ray tracing pipeline // 
	TransitionResource(output, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	commandList->SetPipelineState1(rayTracePipeline->GetPipelineState());
	commandList->DispatchRays(shaderTable->GetDispatchRayDescription());

	TransitionResource(output, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);

	// 2) Copy output from the ray tracing pipeline to the screen buffer //
	TransitionResource(renderTargetBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
	commandList->CopyResource(renderTargetBuffer.Get(), output);
	TransitionResource(renderTargetBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void RayTraceStage::CreateShaderResources()
{
	int width = DXAccess::GetWindow()->GetWindowWidth();
	int height = DXAccess::GetWindow()->GetWindowHeight();

	outputBuffer = new Texture(width, height, DXGI_FORMAT_R8G8B8A8_UNORM);
	colorBuffer = new Texture(width, height, DXGI_FORMAT_R32G32B32A32_FLOAT);

	settingsBuffer = new DXUploadBuffer(&settings, sizeof(PipelineSettings));
}

void RayTraceStage::CreateShaderDescriptors()
{
	// TODO: Consider replacing output & color with just directly mapped resources similar to the current hitgroup
	// root ranges. Same goes for the TLAS, the DXTopLevelAS can also generate a SRV
	ComPtr<ID3D12Device5> device = DXAccess::GetDevice();
	DXDescriptorHeap* heap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	rayGenTableIndex = heap->GetNextAvailableIndex();

	D3D12_CPU_DESCRIPTOR_HANDLE handle = heap->GetCPUHandleAt(rayGenTableIndex);

	D3D12_UNORDERED_ACCESS_VIEW_DESC outputDescription = {};
	outputDescription.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	device->CreateUnorderedAccessView(outputBuffer->GetAddress(), nullptr, &outputDescription, handle);
	
	handle = heap->GetCPUHandleAt(heap->GetNextAvailableIndex());

	D3D12_UNORDERED_ACCESS_VIEW_DESC colorBufferDescription = {};
	colorBufferDescription.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	device->CreateUnorderedAccessView(colorBuffer->GetAddress(), nullptr, &colorBufferDescription, handle);

	handle = heap->GetCPUHandleAt(heap->GetNextAvailableIndex());
}

void RayTraceStage::InitializePipeline()
{
	// TODO: A lot of these descriptor ranges can probably be available for each one of em?
	DXRayTracingPipelineSettings settings;
	settings.maxRayRecursionDepth = 16;

	// RayGen Root //
	CD3DX12_DESCRIPTOR_RANGE rayGenRanges[2];
	rayGenRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0); // Screen 
	rayGenRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0); // Color Buffer 

	CD3DX12_ROOT_PARAMETER rayGenParameters[3];
	rayGenParameters[0].InitAsDescriptorTable(_countof(rayGenRanges), &rayGenRanges[0]);
	rayGenParameters[1].InitAsConstantBufferView(0, 0);
	rayGenParameters[2].InitAsShaderResourceView(0, 0);

	settings.rayGenParameters = &rayGenParameters[0];
	settings.rayGenParameterCount = _countof(rayGenParameters);

	// Hit Root //
	CD3DX12_ROOT_PARAMETER hitParameters[3];
	hitParameters[0].InitAsShaderResourceView(0, 0); // Vertex buffer
	hitParameters[1].InitAsShaderResourceView(1, 0); // Index buffer
	hitParameters[2].InitAsShaderResourceView(2, 0); // TLAS Scene 

	settings.hitParameters = &hitParameters[0];
	settings.hitParameterCount = _countof(hitParameters);

	// Miss Root //
	CD3DX12_DESCRIPTOR_RANGE missRanges[1];
	missRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0); // Screen 

	CD3DX12_ROOT_PARAMETER missParameters[1];
	missParameters[0].InitAsDescriptorTable(_countof(missRanges), &missRanges[0]);

	settings.missParameters = &missParameters[0];
	settings.missParameterCount = _countof(missParameters);

	settings.payLoadSize = sizeof(float) * 5; // RGB, Depth, Seed

	rayTracePipeline = new DXRayTracingPipeline(settings);
}

void RayTraceStage::InitializeShaderBindingTable()
{
	DXDescriptorHeap* heap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	shaderTable = new DXShaderBindingTable(rayTracePipeline->GetPipelineProperties());

	// Common
	auto tlasPtr = reinterpret_cast<UINT64*>(TLAS->GetGPUVirtualAddress());

	auto rayGenTable = reinterpret_cast<UINT64*>(heap->GetGPUHandleAt(rayGenTableIndex).ptr);
	auto settingsPtr = reinterpret_cast<UINT64*>(settingsBuffer->GetGPUVirtualAddress());
	shaderTable->AddRayGenerationProgram(L"RayGen", { rayGenTable, settingsPtr, tlasPtr });

	auto exrPtr = reinterpret_cast<UINT64*>(activeScene->GetEnvironementMap()->GetTexture()->GetSRV().ptr);
	shaderTable->AddMissProgram(L"Miss", { exrPtr });

	const std::vector<Model*>& models = activeScene->GetModels();

	for(Model* model : models)
	{
		const std::vector<Mesh*>& meshes = model->GetMeshes();
		for(Mesh* mesh : meshes)
		{
			auto vertex = reinterpret_cast<UINT64*>(mesh->GetVertexBuffer()->GetGPUVirtualAddress());
			auto index = reinterpret_cast<UINT64*>(mesh->GetIndexBuffer()->GetGPUVirtualAddress());

			shaderTable->AddHitProgram(L"HitGroup", { vertex, index, tlasPtr });
		}
	}

	shaderTable->BuildShaderTable();
}