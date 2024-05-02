#include "Graphics/Texture.h"
#include "Graphics/DXUtilities.h"
#include "Graphics/DXAccess.h"
#include <stb_image.h>

Texture::Texture(void* data, int width, int height, DXGI_FORMAT format, unsigned int formatSizeInBytes) 
	: format(format), formatSizeInBytes(formatSizeInBytes), width(width), height(height)
{
	UploadData(data, width, height);
}

Texture::~Texture()
{
	textureResource.Reset();
}

int Texture::GetWidth()
{
	return width;
}

int Texture::GetHeight()
{
	return height;
}

int Texture::GetSRVIndex()
{
	return srvIndex;
}

int Texture::GetUAVIndex()
{
	return uavIndex;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE Texture::GetSRV()
{
	DXDescriptorHeap* SRVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return SRVHeap->GetGPUHandleAt(srvIndex);
}

CD3DX12_GPU_DESCRIPTOR_HANDLE Texture::GetUAV()
{
	DXDescriptorHeap* SRVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return SRVHeap->GetGPUHandleAt(uavIndex);
}

D3D12_GPU_VIRTUAL_ADDRESS Texture::GetGPULocation()
{
	return textureResource->GetGPUVirtualAddress();
}

DXGI_FORMAT Texture::GetFormat()
{
	return format;
}

ComPtr<ID3D12Resource> Texture::GetResource()
{
	return textureResource;
}

void Texture::UploadData(void* data, int width, int height)
{
	D3D12_RESOURCE_DESC description = CD3DX12_RESOURCE_DESC::Tex2D(
		format, width, height);
	description.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_SUBRESOURCE_DATA subresource;
	subresource.pData = data;
	subresource.RowPitch = width * formatSizeInBytes;

	ComPtr<ID3D12Resource> intermediateTexture;
	UploadPixelShaderResource(textureResource, intermediateTexture, description, subresource);

	DXDescriptorHeap* heap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Create SRV //
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	srvIndex = heap->GetNextAvailableIndex();
	DXAccess::GetDevice()->CreateShaderResourceView(textureResource.Get(), &srvDesc, heap->GetCPUHandleAt(srvIndex));

	// Create UAV //
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	uavIndex = heap->GetNextAvailableIndex();
	DXAccess::GetDevice()->CreateUnorderedAccessView(textureResource.Get(), nullptr, &uavDesc, heap->GetCPUHandleAt(uavIndex));
}