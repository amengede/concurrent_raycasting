#pragma once
#include "config.h"

namespace util {
	unsigned int load_shader(const char* vertexFilepath, const char* geometryFilepath, const char* fragmentFilepath);
	unsigned int load_shader(const char* computeFilepath);
}