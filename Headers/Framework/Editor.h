#pragma once

#include <array>

class Blaze;
class Scene;

class Editor
{
public:
	Editor(Blaze* application, Scene* scene);

	void Update(float deltaTime);

private:
	void Menubar();
	void TransformWindow();

	void ImGuiStyleSettings();

private:
	Blaze* application;
	Scene* activeScene;

	// Timing // 
	float deltaTime;
	std::array<int, 60> averageFPS;
	unsigned int frameCount = 0;

	struct ImFont* baseFont;
	struct ImFont* boldFont;
};