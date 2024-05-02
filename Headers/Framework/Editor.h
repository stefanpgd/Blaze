#pragma once

#include <array>

class Blaze;

class Editor
{
public:
	Editor(Blaze* application);

	void Update(float deltaTime);

private:
	void ImGuiStyleSettings();

private:
	float deltaTime;
	std::array<int, 60> averageFPS;
	unsigned int frameCount = 0;

	struct ImFont* baseFont;
	struct ImFont* boldFont;

	Blaze* application;
};