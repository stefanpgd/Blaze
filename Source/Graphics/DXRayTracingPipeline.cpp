#include "Graphics/DXRayTracingPipeline.h"
#include "Graphics/DXCommon.h"
#include "Graphics/DXUtilities.h"
#include <string>

DXRayTracingPipeline::DXRayTracingPipeline()
{
	CompileShader();
}

void DXRayTracingPipeline::CompileShader()
{
	dxc::DxcDllSupport dxcHelper;
	IDxcCompiler* compiler = nullptr;
	IDxcLibrary* library = nullptr;
	ComPtr<IDxcIncludeHandler> dxcIncludeHandler;

	dxcHelper.Initialize();
	dxcHelper.CreateInstance(CLSID_DxcCompiler, &compiler);
	dxcHelper.CreateInstance(CLSID_DxcLibrary, &library);
	library->CreateIncludeHandler(&dxcIncludeHandler);

	UINT32 code(0);
	IDxcBlobEncoding* pShaderText(nullptr);
	IDxcOperationResult* result;

	// Load and encode the shader file
	std::string path = "Source/Shaders/miss.hlsl";
	std::wstring filePath = std::wstring(path.begin(), path.end());

	ThrowIfFailed(library->CreateBlobFromFile(filePath.c_str(), &code, &pShaderText));
	compiler->Compile(pShaderText, filePath.c_str(), L"main", L"lib_6_3", nullptr, 0, nullptr, 0, nullptr, &result);

	ComPtr<IDxcBlob> test;
	result->GetResult(test.GetAddressOf());
}