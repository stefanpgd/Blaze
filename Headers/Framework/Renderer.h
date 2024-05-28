#pragma once
#include <string>
#include <vector>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class Scene;
class RayTraceStage;

class Renderer
{
public:
	Renderer(const std::wstring& applicationName, unsigned int windowWidth, unsigned int windowHeight);
	
	void InitializeStage(Scene* activeScene);
	void Update(float deltaTime);
	void Render();

	void Resize();

private:
	void InitializeImGui();

private:
	Scene* activeScene;
	RayTraceStage* rayTraceStage;
};