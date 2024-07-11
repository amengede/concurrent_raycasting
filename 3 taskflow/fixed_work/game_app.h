#pragma once
#include "config.h"
#include "scene.h"
#include "engine.h"

enum class returnCode {
	CONTINUE, QUIT
};

class GameApp {
public:
	GameApp(int width, int height);
	void mainLoop();
	~GameApp();
private:
	GLFWwindow* makeWindow();
	returnCode processInput();
	void calculateFrameRate();

	GLFWwindow* window;
	int width, height;
	Scene* scene;
	Engine* renderer;

	double lastTime, currentTime;
	int numFrames;
	float frameTime;
};