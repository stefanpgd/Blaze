#pragma once

#include "Graphics/DXCommon.h"

class Mesh;


class DXTopLevelAS
{
public:
	DXTopLevelAS(Mesh* mesh);

	D3D12_GPU_VIRTUAL_ADDRESS GetTLASAddress();

private:
	void BuildTLAS();

private:
	Mesh* mesh;

	ComPtr<ID3D12Resource> tlasInstanceDesc;
	ComPtr<ID3D12Resource> tlasScratch;
	ComPtr<ID3D12Resource> tlasResult;
};