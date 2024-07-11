#include "engine.h"

Engine::Engine(int width, int height, Scene* scene) {

    raycastDrawShader = util::load_shader("shaders/raycast_vertex.txt", "shaders/raycast_geometry.txt", "shaders/raycast_fragment.txt");
    raycastComputeShader = util::load_shader("shaders/raycast_compute.txt");

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    this->width = width;
    this->height = height;
    this->scene = scene;
    glGenVertexArrays(1, &dummyVAO);

    create_resources();

}

Engine::~Engine() {
    glDeleteVertexArrays(1, &dummyVAO);
    glDeleteProgram(raycastDrawShader);
    glDeleteProgram(raycastComputeShader);
}

void Engine::create_resources() {

    //mapBuffer
    glCreateBuffers(1, &mapBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mapBuffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER,
        scene->worldMap.size() * sizeof(int),
        scene->worldMap.data(), 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mapBuffer);

    //materialBuffer
    glCreateBuffers(1, &materialBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialBuffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER,
        colors.size() * sizeof(glm::vec4),
        colors.data(), 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, materialBuffer);

    //castBuffer
    glCreateBuffers(1, &castBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, castBuffer);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER,
        width * sizeof(glm::vec4),
        NULL, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, castBuffer);

    glUseProgram(raycastComputeShader);
    glUniform2i(glGetUniformLocation(raycastComputeShader, "mapSize"), 24, 24);
    cameraPosLocation = glGetUniformLocation(raycastComputeShader, "cameraPos");
    cameraForwardsLocation = glGetUniformLocation(raycastComputeShader, "cameraForwards");
    cameraRightLocation = glGetUniformLocation(raycastComputeShader, "cameraRight");
}

void Engine::render() {

    glUseProgram(raycastComputeShader);
    glUniform3fv(cameraPosLocation, 1, glm::value_ptr(scene->player->position));
    glUniform3fv(cameraForwardsLocation, 1, glm::value_ptr(scene->player->forwards));
    glUniform3fv(glGetUniformLocation(raycastComputeShader, "cameraRight"), 1, glm::value_ptr(scene->player->right));

    unsigned int workgroup_count = (width + 63) / 64;
    glDispatchCompute(workgroup_count, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(raycastDrawShader);
    glBindVertexArray(dummyVAO);
    glDrawArraysInstanced(GL_POINTS, 0, 1, width);
    glFlush();
}