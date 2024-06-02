#include "Framework/Scene.h"
#include "Graphics/Model.h"
#include "Graphics/EnvironmentMap.h"

Scene::Scene()
{
	// TODO: Remove once there is scene control, for now it acts as the default model(s) //
	AddModel("Assets/Models/GroundPlane/plane.gltf");
	models[0]->transform.Position = glm::vec3(0.0f, -0.55f, 0.0f);

	AddModel("Assets/Models/Dragon/dragon.gltf");
	models[1]->transform.Position = glm::vec3(-1.95f, 0.0f, -2.0f);
	models[1]->transform.Rotation = glm::vec3(0.0f, 40.0f, 0.0f);

	AddModel("Assets/Models/Bunny/bunny.gltf");
	models[2]->transform.Position = glm::vec3(0.0f, -0.6f, 0.0f);
	
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