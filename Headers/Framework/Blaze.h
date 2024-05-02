#pragma once

#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>

#include <string>

class Renderer;
class Editor;

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
	std::wstring applicationName = L"Blaze";
	bool runApplication = true;

	unsigned int windowWidth = 1080;
	unsigned int windowHeight = 720;

	// Systems //
	Renderer* renderer;
	Editor* editor;
};