#pragma once

#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/trigonometric.hpp>
DISABLE_WARNINGS_POP()

#include <filesystem>
#include <stdint.h>

namespace utils {
    // Camera and screen
    constexpr int32_t WIDTH             = 1600;
    constexpr int32_t HEIGHT            = 900;
    constexpr float FOV                 = glm::radians(90.0f);
    constexpr float ASPECT_RATIO        = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    constexpr glm::vec3 START_POSITION  = {0.0f, 0.0f, -3.0f};
    constexpr glm::vec3 START_LOOK_AT   = -START_POSITION;
}
