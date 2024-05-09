#include "Graphics/RenderStages/RayTraceStage.h"
#include "Graphics/DXRayTracingPipeline.h"
#include "Graphics/DXUtilities.h"

RayTraceStage::RayTraceStage(D3D12_GPU_VIRTUAL_ADDRESS tlasAddress)
{
	CreateOutputBuffer();
	CreateShaderResourceHeap(tlasAddress);

	InitializePipeline();
}

void RayTraceStage::RecordStage(ComPtr<ID3D12GraphicsCommandList4> commandList)
{
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

void RayTraceStage::CreateShaderResourceHeap(D3D12_GPU_VIRTUAL_ADDRESS tlasAddress)
{
	rayTraceHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

	ComPtr<ID3D12Device5> device = DXAccess::GetDevice();
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rayTraceHeap->GetCPUHandleAt(0);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDescription = {};
	uavDescription.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	device->CreateUnorderedAccessView(rayTraceOutput.Get(), nullptr, &uavDescription, handle);

	handle = rayTraceHeap->GetCPUHandleAt(1);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location = tlasAddress;
	device->CreateShaderResourceView(nullptr, &srvDesc, handle);
}

void RayTraceStage::InitializePipeline()
{
	DXRayTracingPipelineSettings settings;
	settings.uavSrvHeap = rayTraceHeap;

	CD3DX12_DESCRIPTOR_RANGE rayGenRanges[2];
	rayGenRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
	rayGenRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

	CD3DX12_ROOT_PARAMETER rayGenParameters[1];
	rayGenParameters[0].InitAsDescriptorTable(_countof(rayGenRanges), &rayGenRanges[0]);

	settings.rayGenParameters = &rayGenParameters[0];
	settings.rayGenParameterCount = _countof(rayGenParameters);

	rayTracePipeline = new DXRayTracingPipeline(settings);
}