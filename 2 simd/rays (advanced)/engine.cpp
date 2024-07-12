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
    // rounding up to start from vertically lower pixels
    int block1 = (pixel1 + 7) / 8;
    int padding1 = 8 * block1 - pixel1;

    int pixel2 = y2 + height * x;
    // rounding down to stop at vertically higher pixels
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

    const int blockCount = width / 8;
    const __m256 cameraForwardsX = _mm256_set1_ps(scene->player->forwards.x);
    const __m256 cameraForwardsY = _mm256_set1_ps(scene->player->forwards.y);
    const __m256 cameraRightX = _mm256_set1_ps(scene->player->right.x);
    const __m256 cameraRightY = _mm256_set1_ps(scene->player->right.y);
    __m256 screenXCoords = _mm256_setr_ps(0, 1, 2, 3, 4, 5, 6, 7);
    int x = 0;
    for (int i = 0; i < blockCount; ++i) {

        //Orient the ray
        __m256 horizontalCoefficients = _mm256_fmsub_ps(_mm256_set1_ps(2.0f / width), screenXCoords, _mm256_set1_ps(1));
        __m256 rayXDirections = _mm256_fmadd_ps(cameraRightX, horizontalCoefficients, cameraForwardsX);
        __m256 rayYDirections = _mm256_fmadd_ps(cameraRightY, horizontalCoefficients, cameraForwardsY);
        __m256 rayPosX = _mm256_set1_ps(int(scene->player->position.x));
        __m256 rayPosY = _mm256_set1_ps(int(scene->player->position.y));

        //DDA Parameters
        __m256 zeroMask = _mm256_cmp_ps(rayXDirections, _mm256_setzero_ps(), _CMP_EQ_UQ);
        __m256 negativeMask = _mm256_cmp_ps(rayXDirections, _mm256_setzero_ps(), _CMP_LT_OQ);
        __m256 normalized = _mm256_div_ps(_mm256_set1_ps(1), _mm256_andnot_ps(_mm256_set1_ps(-0.0), rayXDirections));
        __m256 deltaDistX = _mm256_blendv_ps(normalized, _mm256_set1_ps(1e30), zeroMask);
        __m256 stepX = _mm256_blendv_ps(_mm256_set1_ps(1), _mm256_set1_ps(-1), negativeMask);
        __m256 sideDistX = _mm256_mul_ps(deltaDistX,
            _mm256_blendv_ps(
                _mm256_set1_ps(int(scene->player->position.x) + 1.0 - scene->player->position.x),
                _mm256_set1_ps(scene->player->position.x - int(scene->player->position.x)), negativeMask));

        zeroMask = _mm256_cmp_ps(rayYDirections, _mm256_setzero_ps(), _CMP_EQ_UQ);
        negativeMask = _mm256_cmp_ps(rayYDirections, _mm256_setzero_ps(), _CMP_LT_OQ);
        normalized = _mm256_div_ps(_mm256_set1_ps(1), _mm256_andnot_ps(_mm256_set1_ps(-0.0), rayYDirections));
        __m256 deltaDistY = _mm256_blendv_ps(normalized, _mm256_set1_ps(1e30), zeroMask);
        __m256 stepY = _mm256_blendv_ps(_mm256_set1_ps(1), _mm256_set1_ps(-1), negativeMask);
        __m256 sideDistY = _mm256_mul_ps(deltaDistY,
            _mm256_blendv_ps(
                _mm256_set1_ps(int(scene->player->position.y) + 1.0 - scene->player->position.y),
                _mm256_set1_ps(scene->player->position.y - int(scene->player->position.y)), negativeMask));

        __m256 perpWallDist = _mm256_setzero_ps();
        __m256 hit = _mm256_setzero_ps();
        __m256 side = _mm256_setzero_ps();
        __m256i colorBuffer = _mm256_set1_epi32(0);

        //perform DDA
        int done = 0;
        while (done != 255) {
            __m256 hitMask = _mm256_cmp_ps(hit, _mm256_setzero_ps(), _CMP_NEQ_UQ);
            __m256 sideMask = _mm256_cmp_ps(sideDistX, sideDistY, _CMP_LT_OQ);
            sideDistX = _mm256_blendv_ps(_mm256_blendv_ps(sideDistX, _mm256_add_ps(sideDistX, deltaDistX), sideMask), sideDistX, hitMask);
            rayPosX = _mm256_blendv_ps(_mm256_blendv_ps(rayPosX, _mm256_add_ps(rayPosX, stepX), sideMask), rayPosX, hitMask);
            sideDistY = _mm256_blendv_ps(_mm256_blendv_ps(_mm256_add_ps(sideDistY, deltaDistY), sideDistY, sideMask), sideDistY, hitMask);
            rayPosY = _mm256_blendv_ps(_mm256_blendv_ps(_mm256_add_ps(rayPosY, stepY), rayPosY, sideMask), rayPosY, hitMask);

            side = _mm256_blendv_ps(_mm256_blendv_ps(_mm256_set1_ps(1), _mm256_setzero_ps(), sideMask), side, hitMask);
            for (int lane = 0; lane < 8; ++lane) {
                if (done & (1 << lane)) {
                    continue;
                }

                //Check if ray has hit a wall
                int mapX = int(rayPosX.m256_f32[lane]);
                int mapY = int(rayPosY.m256_f32[lane]);
                int materialIndex = scene->worldMap[mapX][mapY];
                if (materialIndex > 0) {

                    //Record hit
                    hit.m256_f32[lane] = 1;
                    done |= 1 << lane;

                    //Record depth
                    perpWallDist.m256_f32[lane] =
                        (side.m256_f32[lane] == 0) ? (sideDistX.m256_f32[lane] - deltaDistX.m256_f32[lane])
                        : perpWallDist.m256_f32[lane] = (sideDistY.m256_f32[lane] - deltaDistY.m256_f32[lane]);

                    //Record color
                    int color = colors[materialIndex];
                    //give x and y sides different brightness
                    if (side.m256_f32[lane] != 0) {
                        color = color >> 1;
                    }
                    colorBuffer.m256i_i32[lane] = color;
                }
            }
        }

        //Calculate height of line to draw on screen
        const __m256 screenHeight = _mm256_set1_ps(height);
        __m256 lineHeight = _mm256_div_ps(screenHeight, perpWallDist);

        //calculate lowest and highest pixel to fill in current stripe
        __m256 drawStart = _mm256_max_ps(_mm256_setzero_ps(),
            _mm256_mul_ps(_mm256_set1_ps(0.5), _mm256_sub_ps(screenHeight, lineHeight)));
        __m256 drawEnd = _mm256_min_ps(_mm256_set1_ps(height - 1),
            _mm256_mul_ps(_mm256_set1_ps(0.5), _mm256_add_ps(screenHeight, lineHeight)));

        for (int lane = 0; lane < 8; ++lane) {
            //draw the pixels of the stripe as a vertical line
            vertical_line(x++, int(drawStart.m256_f32[lane]), int(drawEnd.m256_f32[lane]), colorBuffer.m256i_i32[lane]);
        }
        screenXCoords = _mm256_add_ps(screenXCoords, _mm256_set1_ps(8));
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