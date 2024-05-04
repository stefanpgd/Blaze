#include "Graphics/DXRayTracingPipeline.h"
#include "Graphics/DXUtilities.h"

DXRayTracingPipeline::DXRayTracingPipeline()
{
	// Initialize systems to compile shaders //
	dxcHelper.Initialize();
	dxcHelper.CreateInstance(CLSID_DxcCompiler, &compiler);
	dxcHelper.CreateInstance(CLSID_DxcLibrary, &library);
	library->CreateIncludeHandler(&dxcIncludeHandler);

	// Compile Shaders //
	CompileShader(rayGenerationShader.GetAddressOf(), "rayGeneration.hlsl");
	CompileShader(closestHitShader.GetAddressOf(), "closestHit.hlsl");
	CompileShader(missShader.GetAddressOf(), "miss.hlsl");
}

void DXRayTracingPipeline::CompileShader(IDxcBlob** shader, std::string shaderName)
{
	UINT32 code(0);
	IDxcBlobEncoding* pShaderText(nullptr);
	IDxcOperationResult* result;

	std::string path = "Source/Shaders/" + shaderName;
	std::wstring filePath = std::wstring(path.begin(), path.end());

	ThrowIfFailed(library->CreateBlobFromFile(filePath.c_str(), &code, &pShaderText));

	compiler->Compile(pShaderText, filePath.c_str(), L"main", L"lib_6_3", nullptr, 0, nullptr, 0, nullptr, &result);

	result->GetResult(shader);
}