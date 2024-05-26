#pragma once

#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>

#include <string>

class Renderer;
class Editor;
class Scene;

/// <summary>
/// Information that gets used by the Editor & the ray tracing pipeline
/// </summary>
struct ApplicationInfo
{
	bool clearBuffers = false;
	float time = 1.0f;
	unsigned int frameCount = 0;
	float stub[61];
};

class Blaze
{
public:
	Blaze();

	void Run();

private:
	void RegisterWindowClass();

	void Start();
	void Update(float deltaTime);
	void Render();

	static LRESULT CALLBACK WindowsCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	ApplicationInfo applicationInfo;

	std::wstring applicationName = L"Blaze";
	bool runApplication = true;

	unsigned int windowWidth = 1080;
	unsigned int windowHeight = 720;

	// Systems //
	Renderer* renderer;
	Editor* editor;
	Scene* activeScene;

	friend class Editor;
};