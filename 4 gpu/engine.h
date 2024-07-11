#pragma once
#include "config.h"
#include "scene.h"
#include "shader.h"

struct FrameSize {
	unsigned int width, height;
};

class Engine {
public:
	Engine(int width, int height, Scene* scene);
	~Engine();

	void render();
	void create_resources();


	unsigned int raycastComputeShader, raycastDrawShader;
	unsigned int width, height;
	unsigned int colorBuffer;
	unsigned int mapBuffer, materialBuffer, castBuffer;

	unsigned int cameraPosLocation, cameraForwardsLocation, cameraRightLocation;

	unsigned int dummyVAO;
	Scene* scene;

	std::vector<glm::vec4> colors = { glm::vec4(0.0f),
		glm::vec4(0.0f, 0.0f, 0.5f, 1.0f), glm::vec4(0.0f, 0.5f, 0.0f, 1.0f),
		glm::vec4(0.0f, 0.5f, 0.5f, 1.0f), glm::vec4(0.5f, 0.0f, 0.0f, 1.0f),
		glm::vec4(0.5f, 0.0f, 0.5f, 1.0f) };
};