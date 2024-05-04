#pragma once

#include "Graphics/DXCommon.h"
#include <string>

class DXRayTracingPipeline
{
public:
	DXRayTracingPipeline();

private:
	void CompileShader(IDxcBlob** shader, std::string shaderName);

	// Shaders //
	ComPtr<IDxcBlob> rayGenerationShader;
	ComPtr<IDxcBlob> closestHitShader;
	ComPtr<IDxcBlob> missShader;

	// DX Compiler //
	dxc::DxcDllSupport dxcHelper;
	IDxcCompiler* compiler;
	IDxcLibrary* library;
	ComPtr<IDxcIncludeHandler> dxcIncludeHandler;
};