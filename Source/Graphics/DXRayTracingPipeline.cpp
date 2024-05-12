#include "Graphics/DXRayTracingPipeline.h"
#include "Utilities/Logger.h"
#include "Graphics/DXAccess.h"
#include "Graphics/DXUtilities.h"
#include "Graphics/DXRayTracingUtilities.h"

DXRayTracingPipeline::DXRayTracingPipeline(DXRayTracingPipelineSettings settings) : settings(settings)
{
	// Generate Root Signatures //
	CreateRootSignature(rayGenRootSignature, settings.rayGenParameters, settings.rayGenParameterCount, true);
	CreateRootSignature(hitRootSignature, settings.hitParameters, settings.hitParameterCount, true);
	CreateRootSignature(missRootSignature, settings.missParameters, settings.missParameterCount, true);
	CreateRootSignature(globalDummyRootSignature, nullptr, 0, false);
	CreateRootSignature(localDummyRootSignature, nullptr, 0, true);

	// Compile shaders //
	dxc::DxcDllSupport dxcHelper;
	dxcHelper.Initialize();
	dxcHelper.CreateInstance(CLSID_DxcCompiler, &compiler);
	dxcHelper.CreateInstance(CLSID_DxcLibrary, &library);
	library->CreateIncludeHandler(&dxcIncludeHandler);

	CompileShaderLibrary(rayGenLibrary, L"RayGen.hlsl");
	CompileShaderLibrary(hitLibrary, L"ClosestHit.hlsl");
	CompileShaderLibrary(missLibrary, L"Miss.hlsl");

	// Create pipeline & make shader binding table //
	CreatePipeline();
	CreateShaderBindingTable();
}

ID3D12StateObject* DXRayTracingPipeline::GetPipelineState()
{
	return pipeline.Get();
}

D3D12_DISPATCH_RAYS_DESC* DXRayTracingPipeline::GetDispatchRayDescription()
{
	return &dispatchRayDescription;
}

void DXRayTracingPipeline::CreatePipeline()
{
	std::wstring rayGenSymbol = L"RayGen";
	std::wstring missSymbol = L"Miss";
	std::wstring closestHitSymbol = L"ClosestHit";
	std::wstring hitGroupSymbol = L"HitGroup";

	int objectCount = 15;
	std::vector<D3D12_STATE_SUBOBJECT> subobjects(objectCount);
	unsigned int index = 0;

	AddLibrarySubobject(subobjects, index, rayGenLibrary, &rayGenSymbol);
	AddLibrarySubobject(subobjects, index, missLibrary, &missSymbol);
	AddLibrarySubobject(subobjects, index, hitLibrary, &closestHitSymbol);

	AddHitGroupSubobject(subobjects, index, &hitGroupSymbol, &closestHitSymbol);

	const WCHAR* shaderExports[] = { rayGenSymbol.c_str(), missSymbol.c_str(), closestHitSymbol.c_str() };
	AddShaderPayloadSubobject(subobjects, index, settings.payLoadSize, settings.attributeSize, shaderExports, 3);

	const WCHAR* rayExport[] = { rayGenSymbol.c_str() };
	const WCHAR* missExport[] = { missSymbol.c_str() };
	const WCHAR* hitGroupExport[] = { hitGroupSymbol.c_str() };
	AddRootAssociationSubobject(subobjects, index, rayGenRootSignature, rayExport, 1);
	AddRootAssociationSubobject(subobjects, index, missRootSignature, missExport, 1);
	AddRootAssociationSubobject(subobjects, index, hitRootSignature, hitGroupExport, 1);

	// The pipeline construction always requires an empty global root signature
	AddDummyRootSignatureSubobjects(subobjects, index, globalDummyRootSignature, localDummyRootSignature);

	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
	pipelineConfig.MaxTraceRecursionDepth = settings.maxRayRecursionDepth;

	D3D12_STATE_SUBOBJECT pipelineConfigObject = {};
	pipelineConfigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	pipelineConfigObject.pDesc = &pipelineConfig;

	subobjects[index] = pipelineConfigObject;
	index++;

	// Describe the ray tracing pipeline state object
	D3D12_STATE_OBJECT_DESC pipelineDesc = {};
	pipelineDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	pipelineDesc.NumSubobjects = index; 
	pipelineDesc.pSubobjects = subobjects.data();

	ThrowIfFailed(DXAccess::GetDevice()->CreateStateObject(&pipelineDesc, IID_PPV_ARGS(&pipeline)));
	ThrowIfFailed(pipeline->QueryInterface(IID_PPV_ARGS(&pipelineProperties)));
}

void DXRayTracingPipeline::CreateShaderBindingTable()
{
	// TODO: Consider making some wrapper called SBTBuilder 
	// that takes in calls like 'Bind shader' and responds accordingly

	// The idea of the Shader Binding Table is sort of shaping an array with where all the 
	// information of our pipeline can be found. It's our dictionary
	// For example we know we've shaders that require buffers like the TLAS
	// So where can our program find that? Well we bind the pointer to our descriptorHeap in this program for example

	/*
	All shader records in the Shader Table must have the same size, so shader record size will be based on the largest required entry.
	The ray generation program requires the largest entry:
		32 bytes - D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
	  +  8 bytes - a CBV/SRV/UAV descriptor table pointer (64-bits)
	  = 40 bytes ->> aligns to 64 bytes
	The entry size must be aligned up to D3D12_RAYTRACING_SHADER_BINDING_TABLE_RECORD_BYTE_ALIGNMENT
	*/

	// Step 1 - Figuring out how much bytes we need to allocate for the SBT //
	uint32_t shaderTableSize = 0;
	uint32_t shaderTableRecordSize = 0;
	uint32_t shaderIdSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	shaderTableRecordSize = shaderIdSize;
	shaderTableRecordSize += 8; // UAV-SRV Descriptor Table //
	shaderTableRecordSize += 8; // UAV-SRV Descriptor Table - 2 //

	// Aligns record to be 64-byte, ensuring alignment //
	shaderTableRecordSize = ALIGN(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, shaderTableRecordSize); 

	shaderTableSize = shaderTableRecordSize * 3; // 3 shader entries //
	shaderTableSize = ALIGN(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT, shaderTableSize); // Ensures data is still 64-byte aligned //

	// Step 2 - Allocating memory for the SBT //
	AllocateUploadResource(shaderTable, shaderTableSize);

	// Step 3 - Map data from resources into the SBT //
	uint8_t* pData;
	ThrowIfFailed(shaderTable->Map(0, nullptr, (void**)&pData));

	// Shader Record 0 - Ray Generation program and local root parameter data 
	memcpy(pData, pipelineProperties->GetShaderIdentifier(L"RayGen"), shaderIdSize);

	// Set the root parameter data, Point to start of descriptor heap //
	*reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(pData + shaderIdSize) = settings.uavSrvHeap->GetGPUHandleAt(0);

	// Shader Record 1 - Miss Record 
	pData += shaderTableRecordSize;
	memcpy(pData, pipelineProperties->GetShaderIdentifier(L"Miss"), shaderIdSize);

	// Shader Record 2 - HitGroup Record 
	pData += shaderTableRecordSize;
	memcpy(pData, pipelineProperties->GetShaderIdentifier(L"HitGroup"), shaderIdSize);
	*reinterpret_cast<UINT64*>(pData + shaderIdSize) = settings.vertexBuffer->GetGPUVirtualAddress();
	*reinterpret_cast<UINT64*>(pData + shaderIdSize + 8) = settings.indexBuffer->GetGPUVirtualAddress();

	shaderTable->Unmap(0, nullptr);

	// Step 4 - Use the same information to describe the dispatch Ray
	dispatchRayDescription.RayGenerationShaderRecord.StartAddress = shaderTable->GetGPUVirtualAddress();
	dispatchRayDescription.RayGenerationShaderRecord.SizeInBytes = shaderTableRecordSize;

	dispatchRayDescription.MissShaderTable.StartAddress = shaderTable->GetGPUVirtualAddress() + shaderTableRecordSize;
	dispatchRayDescription.MissShaderTable.SizeInBytes = shaderTableRecordSize;
	dispatchRayDescription.MissShaderTable.StrideInBytes = shaderTableRecordSize;

	dispatchRayDescription.HitGroupTable.StartAddress = shaderTable->GetGPUVirtualAddress() + (shaderTableRecordSize * 2);
	dispatchRayDescription.HitGroupTable.SizeInBytes = shaderTableRecordSize;
	dispatchRayDescription.HitGroupTable.StrideInBytes = shaderTableRecordSize;

	dispatchRayDescription.Width = DXAccess::GetWindow()->GetWindowWidth();
	dispatchRayDescription.Height = DXAccess::GetWindow()->GetWindowHeight();
	dispatchRayDescription.Depth = 1;
}

void DXRayTracingPipeline::CreateRootSignature(ComPtr<ID3D12RootSignature>& rootSignature,
	D3D12_ROOT_PARAMETER* parameterData, unsigned int parameterCount, bool isLocal)
{
	D3D12_ROOT_SIGNATURE_DESC rootDesc = {};
	rootDesc.pParameters = parameterData;
	rootDesc.NumParameters = parameterCount;
	rootDesc.Flags = isLocal ? D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE : D3D12_ROOT_SIGNATURE_FLAG_NONE; 

	ID3DBlob* pSigBlob;
	ID3DBlob* pErrorBlob;
	HRESULT hr = D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &pSigBlob, &pErrorBlob);

	if(FAILED(hr))
	{
		if(pErrorBlob)
		{
			std::string buffer = std::string((char*)pErrorBlob->GetBufferPointer());
			LOG(Log::MessageType::Error, buffer);
		}

		assert(false && "Compilation of root signature failed, read console for errors.");
	}

	ComPtr<ID3D12Device5> device = DXAccess::GetDevice();
	ThrowIfFailed(device->CreateRootSignature(0, pSigBlob->GetBufferPointer(), 
		pSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}

void DXRayTracingPipeline::CompileShaderLibrary(ComPtr<IDxcBlob>& shaderLibrary, std::wstring shaderName)
{
	UINT32 code(0);
	IDxcBlobEncoding* pShaderText(nullptr);
	IDxcOperationResult* result;

	std::wstring path = L"Source/Shaders/" + shaderName;
	std::wstring filePath = std::wstring(path.begin(), path.end());

	ThrowIfFailed(library->CreateBlobFromFile(filePath.c_str(), &code, &pShaderText));

	ThrowIfFailed(compiler->Compile(pShaderText, filePath.c_str(), L"", L"lib_6_3", nullptr, 0, nullptr, 0, dxcIncludeHandler, &result));

	ThrowIfFailed(result->GetResult(&shaderLibrary));
}