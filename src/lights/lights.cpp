#include "lights.h"

#include <constants.h>

#include <utility>

using namespace lum;

Light::Light(Window *window, glm::vec3 position, glm::vec4 color)
    : m_camera(Camera(window, position, utils::START_LOOK_AT)), m_color(color) {}

glm::mat4 Light::lightMatrix() {
    return m_camera.viewMatrix();
}

std::string Light::toString() const {
    return "|R " + std::to_string(m_color.r).substr(0, 5) + " |G" + std::to_string(m_color.g).substr(0, 5) + " |B " + std::to_string(m_color.b).substr(0, 5);
}


LightManager::LightManager(std::vector<Light> lights)
    : m_lights(std::move(lights)) {
    // Generate BOs
    glGenVertexArrays(1, &m_VBO);
    glGenVertexArrays(1, &m_VAO);
}

Light &LightManager::crtLight() {
    return m_lights[m_selectedLightIndex];
}

void LightManager::refreshUBOs() {
    int number_of_lights = static_cast<int>(m_lights.size());
    glGenBuffers(1, &m_UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, m_UBO);
    //Set buffer size
    glBufferData(GL_UNIFORM_BUFFER, number_of_lights * sizeof(LightShadingData) + 16, NULL, GL_DYNAMIC_DRAW);
    //Number of lights goes into first 4 bytes
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &number_of_lights);

    // Generate the light data (only position and color)
    std::vector<LightShadingData> light_shading_data;
    for (const auto& light : m_lights) {
        light_shading_data.push_back(LightShadingData(glm::vec4(light.m_camera.m_position, 1.0), light.m_color));
    }
    //Now the light data
    glBufferSubData(GL_UNIFORM_BUFFER, 16, number_of_lights * sizeof(LightShadingData), light_shading_data.data());
    //Unbind buffer
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void LightManager::refreshVBOs() {
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, m_lights.size() * sizeof(glm::vec4), m_lights.data(), GL_DYNAMIC_DRAW);
}


void LightManager::generateVAOs() {
    glBindVertexArray(m_VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    
    glBufferData(GL_ARRAY_BUFFER, m_lights.size() * sizeof(glm::vec4), m_lights.data(), GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Light), (void *)(offsetof(Light, m_camera) + offsetof(Camera, m_position)));
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
