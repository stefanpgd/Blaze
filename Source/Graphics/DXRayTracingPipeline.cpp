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

	// Create pipeline & get properties //
	CreatePipeline();
}

void DXRayTracingPipeline::CreatePipeline()
{
	std::vector<D3D12_STATE_SUBOBJECT> subobjects;
	unsigned int objectCount = 0;

	std::wstring rayGenSymbol = L"RayGen";
	std::wstring closestHitSymbol = L"ClosestHit";
	std::wstring missSymbol = L"Miss";
	std::wstring hitGroupSymbol = L"HitGroup";

	//AddLibrarySubobject(subobjects, objectCount, rayGenLibrary.Get(), &rayGenSymbol);
	//AddLibrarySubobject(subobjects, objectCount, hitLibrary.Get(), &closestHitSymbol);
	//AddLibrarySubobject(subobjects, objectCount, missLibrary.Get(), &missSymbol);

	//AddHitGroupSubobject(subobjects, objectCount, &hitGroupSymbol, &closestHitSymbol);

	//AddShaderPlayloadSubobject(subobjects, objectCount, sizeof(float) * 4,
	//sizeof(float) * 2, rayGenSymbol, missSymbol, hitGroupSymbol);

		// Add a state subobject for the shader payload configuration
	//D3D12_RAYTRACING_SHADER_CONFIG shaderDesc = {};
	//shaderDesc.MaxPayloadSizeInBytes = sizeof(float) * 4;
	//shaderDesc.MaxAttributeSizeInBytes = sizeof(float) * 2;
	//
	//D3D12_STATE_SUBOBJECT shaderConfigObject = {};
	//shaderConfigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	//shaderConfigObject.pDesc = &shaderDesc;
	//
	//subobjects.push_back(shaderConfigObject);
	//objectCount++;

	// Create a list of the shader export names that use the payload
	//const WCHAR* shaderExports[] = { rayGenSymbol.c_str(), missSymbol.c_str(), hitGroupSymbol.c_str() };
	//
	//// Add a state subobject for the association between shaders and the payload
	//D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderPayloadAssociation = {};
	//shaderPayloadAssociation.NumExports = _countof(shaderExports);
	//shaderPayloadAssociation.pExports = shaderExports;
	//shaderPayloadAssociation.pSubobjectToAssociate = &subobjects[(objectCount - 1)];
	//
	//D3D12_STATE_SUBOBJECT shaderPayloadAssociationObject = {};
	//shaderPayloadAssociationObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	//shaderPayloadAssociationObject.pDesc = &shaderPayloadAssociation;
	//
	//subobjects.push_back(shaderPayloadAssociationObject);
	//objectCount++;

	//AddRootAssociationSubobject(subobjects, objectCount, rayGenRootSignature, L"RayGen");
	//AddRootAssociationSubobject(subobjects, objectCount, missRootSignature, L"Miss");
	//AddRootAssociationSubobject(subobjects, objectCount, hitRootSignature, L"HitGroup");

	D3D12_STATE_SUBOBJECT globalRootSig;
	globalRootSig.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	ID3D12RootSignature* dgSig = globalDummyRootSignature.Get();
	globalRootSig.pDesc = &dgSig;

	subobjects.push_back(globalRootSig);
	objectCount++;

	// The pipeline construction always requires an empty local root signature
	D3D12_STATE_SUBOBJECT dummyLocalRootSig;
	dummyLocalRootSig.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	ID3D12RootSignature* dlSig = localDummyRootSignature.Get();
	dummyLocalRootSig.pDesc = &dlSig;
	
	subobjects.push_back(dummyLocalRootSig);
	objectCount++;

	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
	pipelineConfig.MaxTraceRecursionDepth = 1;

	D3D12_STATE_SUBOBJECT pipelineConfigObject = {};
	pipelineConfigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	pipelineConfigObject.pDesc = &pipelineConfig;

	subobjects.push_back(pipelineConfigObject);
	objectCount++;

	// Describe the ray tracing pipeline state object
	D3D12_STATE_OBJECT_DESC pipelineDesc = {};
	pipelineDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	pipelineDesc.NumSubobjects = objectCount; 
	pipelineDesc.pSubobjects = subobjects.data();

	ThrowIfFailed(DXAccess::GetDevice()->CreateStateObject(&pipelineDesc, IID_PPV_ARGS(&pipeline)));
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
		std::string buffer = std::string((char*)pErrorBlob->GetBufferPointer());
		LOG(Log::MessageType::Error, buffer);
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