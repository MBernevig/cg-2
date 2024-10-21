#pragma once

#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/trigonometric.hpp>
DISABLE_WARNINGS_POP()

#include <filesystem>
#include <stdint.h>

namespace utils {
    // Camera and screen
    constexpr int32_t WIDTH             = 1280;
    constexpr int32_t HEIGHT            = 720;
    constexpr float FOV                 = glm::radians(70.0f);
    constexpr float ASPECT_RATIO        = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    constexpr glm::vec3 START_POSITION  = {0.0f, 0.0f, -3.0f};
    constexpr glm::vec3 START_LOOK_AT   = -START_POSITION;

    // Light
    struct Light {
        glm::vec3 position;
        glm::vec3 color;
        bool is_spotlight;
        glm::vec3 direction;
        bool has_texture;
        // Assuming Texture is a defined type elsewhere in your code
        Texture texture;
    };

    Light DEFAULT_LIGHT = {
        {0.0f, 1.0f, 0.0f},  // position
        {1.0f, 1.0f, 1.0f},  // color
        false,               // is_spotlight
        {0.0f, -1.0f, 0.0f}, // direction
        false,               // has_texture
        {RESOURCE_ROOT "resources/checkerboard.png"}                   // texture (default constructed)
    };
}
