#include "Graphics/RenderStages/RayTraceStage.h"
#include "Graphics/DXRayTracingPipeline.h"

#include "Graphics/DXUtilities.h"
#include "Graphics/DXTopLevelAS.h"
#include "Graphics/Mesh.h"

RayTraceStage::RayTraceStage(Mesh* mesh) : mesh(mesh)
{
	TLAS = new DXTopLevelAS(mesh);
	
	CreateOutputBuffer();
	CreateColorBuffer();

	// TODO: Temporarily here, move it to something proper
	AllocateAndMapResource(settingsBuffer, &settings, sizeof(RayTraceSettings));

	CreateShaderResourceHeap();


	InitializePipeline();
}

void RayTraceStage::RecordStage(ComPtr<ID3D12GraphicsCommandList4> commandList)
{
	// TODO: Move to an update call
	settings.time += 0.0068;
	settings.frameCount++;

	UpdateUploadHeapResource(settingsBuffer, &settings, sizeof(RayTraceSettings));

	// Resources //
	ComPtr<ID3D12Resource> renderTargetBuffer = DXAccess::GetWindow()->GetCurrentScreenBuffer();

	ID3D12DescriptorHeap* heaps[] = { rayTraceHeap->GetAddress() };
	commandList->SetDescriptorHeaps(1, heaps);

	TransitionResource(rayTraceOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	commandList->SetPipelineState1(rayTracePipeline->GetPipelineState());
	commandList->DispatchRays(rayTracePipeline->GetDispatchRayDescription());

	TransitionResource(rayTraceOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);

	TransitionResource(renderTargetBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
	commandList->CopyResource(renderTargetBuffer.Get(), rayTraceOutput.Get());
	TransitionResource(renderTargetBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void RayTraceStage::CreateOutputBuffer()
{
	ComPtr<ID3D12Device5> device = DXAccess::GetDevice();
	Window* window = DXAccess::GetWindow();

	D3D12_RESOURCE_DESC resourceDescription = {};
	resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resourceDescription.DepthOrArraySize = 1;
	resourceDescription.MipLevels = 1;
	resourceDescription.SampleDesc.Count = 1;

	resourceDescription.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resourceDescription.Width = window->GetWindowWidth();
	resourceDescription.Height = window->GetWindowHeight();
	resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	CD3DX12_HEAP_PROPERTIES defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &resourceDescription,
		 D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&rayTraceOutput));
}

void RayTraceStage::CreateColorBuffer()
{
	ComPtr<ID3D12Device5> device = DXAccess::GetDevice();
	Window* window = DXAccess::GetWindow();

	D3D12_RESOURCE_DESC resourceDescription = {};
	resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDescription.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	resourceDescription.DepthOrArraySize = 1;
	resourceDescription.MipLevels = 1;
	resourceDescription.SampleDesc.Count = 1;

	resourceDescription.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resourceDescription.Width = window->GetWindowWidth();
	resourceDescription.Height = window->GetWindowHeight();
	resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	CD3DX12_HEAP_PROPERTIES defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &resourceDescription,
		 D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&colorBuffer));
}

void RayTraceStage::CreateShaderResourceHeap()
{
	rayTraceHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

	ComPtr<ID3D12Device5> device = DXAccess::GetDevice();
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rayTraceHeap->GetCPUHandleAt(0);

	D3D12_UNORDERED_ACCESS_VIEW_DESC outputDescription = {};
	outputDescription.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	device->CreateUnorderedAccessView(rayTraceOutput.Get(), nullptr, &outputDescription, handle);

	handle = rayTraceHeap->GetCPUHandleAt(1);

	D3D12_UNORDERED_ACCESS_VIEW_DESC colorBufferDescription = {};
	colorBufferDescription.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	device->CreateUnorderedAccessView(colorBuffer.Get(), nullptr, &colorBufferDescription, handle);

	handle = rayTraceHeap->GetCPUHandleAt(2);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location = TLAS->GetTLASAddress();
	device->CreateShaderResourceView(nullptr, &srvDesc, handle);

	handle = rayTraceHeap->GetCPUHandleAt(3);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = settingsBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = sizeof(RayTraceSettings);
	device->CreateConstantBufferView(&cbvDesc, handle);
}

void RayTraceStage::InitializePipeline()
{
	DXRayTracingPipelineSettings settings;
	settings.uavSrvHeap = rayTraceHeap;
	settings.vertexBuffer = mesh->GetVertexBuffer();
	settings.indexBuffer = mesh->GetIndexBuffer();
	settings.maxRayRecursionDepth = 12;
	settings.TLAS = TLAS;

	CD3DX12_DESCRIPTOR_RANGE rayGenRanges[4];
	rayGenRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0); // Screen 
	rayGenRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, 0); // Color Buffer 
	rayGenRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0); // Acceleration Structure 
	rayGenRanges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0); // General Settings 

	CD3DX12_ROOT_PARAMETER rayGenParameters[1];
	rayGenParameters[0].InitAsDescriptorTable(_countof(rayGenRanges), &rayGenRanges[0]);

	settings.rayGenParameters = &rayGenParameters[0];
	settings.rayGenParameterCount = _countof(rayGenParameters);

	CD3DX12_ROOT_PARAMETER hitParameters[3];
	hitParameters[0].InitAsShaderResourceView(0, 0); // Vertex buffer
	hitParameters[1].InitAsShaderResourceView(1, 0); // Index buffer
	hitParameters[2].InitAsShaderResourceView(2, 0); // TLAS Scene 

	settings.hitParameters = &hitParameters[0];
	settings.hitParameterCount = _countof(hitParameters);

	settings.payLoadSize = sizeof(float) * 5; // RGB, Depth, Seed

	rayTracePipeline = new DXRayTracingPipeline(settings);
}