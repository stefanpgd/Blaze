#include "Framework/Scene.h"
#include "Graphics/Model.h"
#include "Graphics/EnvironmentMap.h"

Scene::Scene()
{
	AddModel("Assets/Models/Bust/marble_bust_01_4k.gltf");
	models[0]->transform.Position = glm::vec3(-0.79f, -0.70f, 3.65f);
	models[0]->transform.Rotation = glm::vec3(0.0f, 37.0f, 0.0f);
	models[0]->transform.Scale = glm::vec3(2.25f);

	AddModel("Assets/Models/Chess/chess_set_2k.gltf");
	models[1]->transform.Position = glm::vec3(0.5f, -0.70f, 3.15f);
	models[1]->transform.Rotation = glm::vec3(0.0f, 33.0f, 0.0f);
	models[1]->transform.Scale = glm::vec3(3.0f);

	AddModel("Assets/Models/GroundPlane/plane.gltf");
	models[2]->transform.Position = glm::vec3(0.0f, -0.70f, 0.0);

	//AddModel("Assets/Models/Sphere/sphere.gltf");

	// Environment Map //
	std::string exrPath = "Assets/EXRs/wharf.exr";
	environmentMap = new EnvironmentMap(exrPath);
}

void Scene::AddModel(const std::string& path)
{
	models.push_back(new Model(path, true));
}

const std::vector<Model*>& Scene::GetModels()
{
	return models;
}

EnvironmentMap* const Scene::GetEnvironementMap()
{
	return environmentMap;
}