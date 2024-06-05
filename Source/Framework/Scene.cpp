#include "Framework/Scene.h"
#include "Graphics/Model.h"
#include "Graphics/EnvironmentMap.h"

Scene::Scene()
{
	//AddModel("Assets/Models/FlightHelmet/FlightHelmet.gltf");
	//models[0]->transform.Position = glm::vec3(0.8f, -0.55f, 3.0f);
	//models[0]->transform.Rotation = glm::vec3(0.0f, -30.0f, 0.0f);
	//models[0]->transform.Scale = glm::vec3(2.0f, 2.0f, 2.0f);

	//AddModel("Assets/Models/GroundPlane/plane.gltf");
	//models[1]->transform.Position = glm::vec3(0.0f, -0.55f, 0.0f);
	//
	AddModel("Assets/Models/Dragon/dragon.gltf");
	//models[2]->transform.Rotation = glm::vec3(0.0f, 40.0f, 0.0f);
	//
	//AddModel("Assets/Models/Bunny/bunny.gltf");
	//models[3]->transform.Position = glm::vec3(-1.05f, -0.6f, -0.75f);
	//models[3]->transform.Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	//models[3]->transform.Scale = glm::vec3(0.75f, 0.75f, 0.75f);
	//
	//AddModel("Assets/Models/MaterialKnob/materialKnob.gltf");
	//models[4]->transform.Position = glm::vec3(-0.65f, -0.537f, 3.75f);
	//models[4]->transform.Rotation = glm::vec3(0.0f, 32.5f, 0.0f);
	//models[4]->transform.Scale = glm::vec3(0.25f, 0.25f, 0.25f);

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