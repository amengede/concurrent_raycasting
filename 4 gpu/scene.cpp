#include "scene.h"

Scene::Scene() {

	PlayerCreateInfo playerInfo;
	playerInfo.eulers = { 0.0f, 90.0f, 180.0f };
	playerInfo.position = { 22.0f, 12.0f, 0.0f };
	player = new Player(&playerInfo);

}

Scene::~Scene() {
	delete player;
}

void Scene::update(float rate) {

	player->update();
}

void Scene::movePlayer(glm::vec3 dPos) {

	if (worldMap[int(player->position.x +  dPos.x) * 24 + int(player->position.y)] == 0) {
		player->position.x += dPos.x;
	}
	if (worldMap[int(player->position.x) * 24 + int(player->position.y + dPos.y)] == 0) {
		player->position.y += dPos.y;
	}

}

void Scene::spinPlayer(glm::vec3 dEulers) {
	player->eulers += dEulers;

	if (player->eulers.z < 0) {
		player->eulers.z += 360;
	}
	else if (player->eulers.z > 360) {
		player->eulers.z -= 360;
	}

	player->eulers.y = std::max(std::min(player->eulers.y, 179.0f), 1.0f);
}