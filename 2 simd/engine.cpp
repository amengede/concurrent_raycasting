#include "engine.h"

Engine::Engine(int width, int height) {

    shader = util::load_shader("shaders/vertex.txt", "shaders/fragment.txt");
    glUseProgram(shader);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    this->width = width;
    this->height = height;
    screenMesh = new QuadModel;

    create_color_buffer(width, height);

}

Engine::~Engine() {
    delete screenMesh;
    glDeleteTextures(1, &colorBuffer);
    glDeleteProgram(shader);
}

void Engine::create_color_buffer(int width, int height) {

    glGenTextures(1, &colorBuffer);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);

    glTextureParameteri(colorBuffer, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(colorBuffer, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(colorBuffer, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(colorBuffer, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    colorBufferMemory.resize(width * height);

}

void Engine::clear_screen(uint32_t color) {

    __m256i colorSIMD = _mm256_set1_epi32(color);
    int blockCount = static_cast<int>(width * height / 8);
    __m256i* blocks = (__m256i*) colorBufferMemory.data();

    //SIMD as much as possible
    for (int block = 0; block < blockCount; ++block) {
        blocks[block] = colorSIMD;
    }

    //set any remaining pixels individually
    for (int pixel = blockCount * 8; pixel < width * height; ++pixel) {
        colorBufferMemory[pixel] = color;
    }
}

void Engine::vertical_line(int x, int y1, int y2, uint32_t color){

    __m256i colorSIMD = _mm256_set1_epi32(color);
    int blockCount = static_cast<int>(width * height / 8);
    __m256i* blocks = (__m256i*) colorBufferMemory.data();

    //get block indices and padding
    int pixel1 = y1 + height * x;
    int block1 = (pixel1 + 7) / 8;
    int padding1 = 8 * block1 - pixel1;

    int pixel2 = y2 + height * x;
    int block2 = pixel2 / 8;
    int padding2 = pixel2 - 8 * block2;

    //bottom
    for (int y = y1; y <= y1 + padding1; ++y) {
        colorBufferMemory[y + height * x] = color;
    }

    for (int block = block1; block < block2; ++block) {
        blocks[block] = colorSIMD;
    }

    //top
    for (int y = y2 - padding2; y <= y2; ++y) {
        colorBufferMemory[y + height * x] = color;
    }
}

void Engine::pset(int x, int y, glm::vec3 color) {
    uint8_t r = std::max(0, std::min(255, (int)(255 * color.x)));
    uint8_t g = std::max(0, std::min(255, (int)(255 * color.y)));
    uint8_t b = std::max(0, std::min(255, (int)(255 * color.z)));
    colorBufferMemory[y + height * x] = (r << 24) + (g << 8) + (b << 16);
}

void Engine::render(Scene* scene) {

    clear_screen(0);

    for (int x = 0; x < width; ++x)
    {
        float cameraX = 2 * x / (float)width - 1;
        float rayDirX = scene->player->forwards.x + scene->player->right.x * cameraX;
        float rayDirY = scene->player->forwards.y + scene->player->right.y * cameraX;
        //which box of the map we're in
        int mapX = int(scene->player->position.x);
        int mapY = int(scene->player->position.y);

        //length of ray from current position to next x or y-side
        float sideDistX;
        float sideDistY;

        float deltaDistX = (rayDirX == 0) ? 1e30 : std::abs(1 / rayDirX);
        float deltaDistY = (rayDirY == 0) ? 1e30 : std::abs(1 / rayDirY);

        float perpWallDist;

        //what direction to step in x or y-direction (either +1 or -1)
        int stepX;
        int stepY;

        int hit = 0; //was there a wall hit?
        int side; //was a NS or a EW wall hit?
        //calculate step and initial sideDist
        if (rayDirX < 0)
        {
            stepX = -1;
            sideDistX = (scene->player->position.x - mapX) * deltaDistX;
        }
        else
        {
            stepX = 1;
            sideDistX = (mapX + 1.0 - scene->player->position.x) * deltaDistX;
        }
        if (rayDirY < 0)
        {
            stepY = -1;
            sideDistY = (scene->player->position.y - mapY) * deltaDistY;
        }
        else
        {
            stepY = 1;
            sideDistY = (mapY + 1.0 - scene->player->position.y) * deltaDistY;
        }
        //perform DDA
        while (hit == 0)
        {
            //jump to next map square, either in x-direction, or in y-direction
            if (sideDistX < sideDistY)
            {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0;
            }
            else
            {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1;
            }
            //Check if ray has hit a wall
            if (scene->worldMap[mapX][mapY] > 0) hit = 1;
        }
        if (side == 0) perpWallDist = (sideDistX - deltaDistX);
        else          perpWallDist = (sideDistY - deltaDistY);

        //Calculate height of line to draw on screen
        int lineHeight = (int)(height / perpWallDist);

        //calculate lowest and highest pixel to fill in current stripe
        int drawStart = -lineHeight / 2 + height / 2;
        if (drawStart < 0) drawStart = 0;
        int drawEnd = lineHeight / 2 + height / 2;
        if (drawEnd >= height) drawEnd = height - 1;

        //choose wall color
        int color = colors[scene->worldMap[mapX][mapY]];

        //give x and y sides different brightness
        if (side == 1) {
            color = color >> 1;
        }

        //draw the pixels of the stripe as a vertical line
        vertical_line(x, drawStart, drawEnd, color);
    }

    draw_screen();
}

void Engine::draw_screen() {

    glUseProgram(shader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, height, width, 0, GL_RGBA, GL_UNSIGNED_BYTE, colorBufferMemory.data());

    glBindVertexArray(screenMesh->VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glFlush();

}