#pragma once

#include <string>
#include <vector>

class Model;
class EnvironmentMap;

/// <summary>
/// Responsible for owning and managing all the geometry in a Scene
/// If a model needs to be loaded, it happens through the Scene.
/// Later on functionality such as serialization should be connected up via here.
/// </summary>
class Scene
{
public:
	Scene();

	void AddModel(const std::string& path);

	const std::vector<Model*>& GetModels();
	EnvironmentMap* const GetEnvironementMap();

public:
	bool HasGeometryMoved = false;
	bool HasNewGeometry = false;

private:
	std::string sceneName;
	std::vector<Model*> models;
	EnvironmentMap* environmentMap;

	friend class Editor;
};