#pragma once
#include "config.h"
#include "scene.h"
#include "shader.h"
#include "quad_model.h"

struct FrameSize {
	unsigned int width, height;
};

class Engine {
public:
	Engine(int width, int height);
	~Engine();

	void render(Scene* scene);
	void create_color_buffer(int width, int height);
	void draw_screen();
	void pset(int x, int y, glm::vec3 color);
	void vertical_line(int x, int y1, int y2, uint32_t color);
	void clear_screen(uint32_t color);


	unsigned int shader, width, height;
	unsigned int colorBuffer;
	std::vector<uint32_t> colorBufferMemory;
	QuadModel* screenMesh;

	uint32_t colors[6] = {
		static_cast < uint32_t>(0),
		static_cast<uint32_t>((0 << 24) + (0 << 16) + (128 << 8) + 255),
		static_cast<uint32_t>((0 << 24) + (128 << 16) + (0 << 8) + 255),
		static_cast<uint32_t>((0 << 24) + (128 << 16) + (128 << 8) + 255),
		static_cast<uint32_t>((128 << 24) + (0 << 16) + (0 << 8) + 255),
		static_cast<uint32_t>((128 << 24) + (0 << 16) + (128 << 8) + 255)
	};
};