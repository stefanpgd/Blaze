#include "Framework/Scene.h"
#include "Graphics/Model.h"

Scene::Scene()
{
	// TODO: Remove once there is scene control, for now it acts as the default model(s) //
	AddModel("Assets/Models/Dragon/dragon.gltf");
	
	AddModel("Assets/Models/Dragon/dragon.gltf");
	models[1]->transform.Position = glm::vec3(-1.95f, 0.0f, -2.0f);
	models[1]->transform.Rotation = glm::vec3(0.0f, 40.0f, 0.0f);

	AddModel("Assets/Models/Dragon/dragon.gltf");
	models[2]->transform.Position = glm::vec3(2.15f, 0.5f, -2.0f);
	models[2]->transform.Rotation = glm::vec3(0.0f, 35.0f, 0.0f);
}

void Scene::AddModel(const std::string& path)
{
	models.push_back(new Model(path, true));
}

const std::vector<Model*>& Scene::GetModels()
{
	return models;
}