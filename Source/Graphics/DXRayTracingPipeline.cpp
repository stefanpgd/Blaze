#include "Graphics/DXRayTracingPipeline.h"
#include "Utilities/Logger.h"
#include "Graphics/DXAccess.h"
#include "Graphics/DXUtilities.h"

DXRayTracingPipeline::DXRayTracingPipeline(DXRayTracingPipelineSettings settings) : settings(settings)
{
	// Generate Root Signatures //
	CreateRootSignature(rayGenRootSignature.GetAddressOf(), settings.rayGenParameters, settings.rayGenParameterCount);
	CreateRootSignature(hitRootSignature.GetAddressOf(), settings.hitParameters, settings.hitParameterCount);
	CreateRootSignature(missRootSignature.GetAddressOf(), settings.missParameters, settings.missParameterCount);

	// Compile shaders //
	dxc::DxcDllSupport dxcHelper;
	dxcHelper.Initialize();
	dxcHelper.CreateInstance(CLSID_DxcCompiler, &compiler);
	dxcHelper.CreateInstance(CLSID_DxcLibrary, &library);
	library->CreateIncludeHandler(&dxcIncludeHandler);

	CompileShaderLibrary(rayGenLibrary.GetAddressOf(), L"RayGeneration.hlsl");
	CompileShaderLibrary(hitLibrary.GetAddressOf(), L"ClosestHit.hlsl");
	CompileShaderLibrary(missLibrary.GetAddressOf(), L"Miss.hlsl");
}

void DXRayTracingPipeline::CreateRootSignature(ID3D12RootSignature** rootSignature, 
	D3D12_ROOT_PARAMETER* parameterData, unsigned int parameterCount)
{
	D3D12_ROOT_SIGNATURE_DESC rootDesc = {};
	rootDesc.pParameters = parameterData;
	rootDesc.NumParameters = parameterCount;
	rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE; // For ray tracing pipelines, root signatures are local

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
		pSigBlob->GetBufferSize(), IID_PPV_ARGS(rootSignature)));
}

void DXRayTracingPipeline::CompileShaderLibrary(IDxcBlob** shader, std::wstring shaderName)
{
	UINT32 code(0);
	IDxcBlobEncoding* pShaderText(nullptr);
	IDxcOperationResult* result;

	std::wstring path = L"Source/Shaders/" + shaderName;
	std::wstring filePath = std::wstring(path.begin(), path.end());

	ThrowIfFailed(library->CreateBlobFromFile(filePath.c_str(), &code, &pShaderText));

	compiler->Compile(pShaderText, filePath.c_str(), L"", L"lib_6_3", nullptr, 0, nullptr, 0, nullptr, &result);
	result->GetResult(shader);
}