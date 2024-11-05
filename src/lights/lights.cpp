#include "lights.h"

#include <camera.h>
#include <camera.h>
#include <camera.h>
#include <camera.h>
#include <camera.h>
#include <camera.h>
#include <camera.h>
#include <constants.h>

#include <glm/gtc/type_ptr.hpp>

using namespace lum;

Light::Light(Window *window, glm::vec3 position, glm::vec4 color)
    : m_camera(Camera(window, position, utils::START_LOOK_AT)), m_color(color) {
    // initialize FBO / depth texture
    glGenTextures(1, &m_shadowMap);
    glBindTexture(GL_TEXTURE_2D, m_shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
                 nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenFramebuffers(1, &m_shadowMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
}


std::string Light::toString() const {
    return "|R " + std::to_string(m_color.r).substr(0, 5) + " |G" + std::to_string(m_color.g).substr(0, 5) + " |B " +
           std::to_string(m_color.b).substr(0, 5);
}

void Light::drawShadowMap(const Shader &shadowShader, const glm::mat4 &mvpMatrix, std::vector<GPUMesh> &meshes) {

    m_mvp = mvpMatrix;

    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFBO);

    // clear shadow map
    glClearDepth(1.0);
    glClear(GL_DEPTH_BUFFER_BIT);

    for (auto &mesh: meshes) {
        shadowShader.bind();
        glm::mat4 mvp = mvpMatrix * mesh.modelMatrix;
        glUniformMatrix4fv(shadowShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvp));
        mesh.draw(shadowShader);
    }
}


LightManager::LightManager(std::vector<Light> lights)
    : m_lights(std::move(lights)) {
    // Generate objects
    glGenBuffers(1, &m_VBO);
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_UBO);

    refreshVBOs(); // set them the first time around

    // init vao only once
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), static_cast<void *>(0));
    glEnableVertexAttribArray(0);

    // init the shadow map array
    // ! - max 10 lights for now, I don't have the brainpower to make dynamic texture arrays
    glGenTextures(1, &m_shadowMapArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_shadowMapArray);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, Light::SHADOW_WIDTH, Light::SHADOW_HEIGHT, 10, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
}

Light &LightManager::crtLight() {
    return m_lights[m_selectedLightIndex];
}

void LightManager::drawLights(const Shader &lightShader, const glm::mat4 &mvp) {
    lightShader.bind();
    refreshVBOs();

    glEnable(GL_PROGRAM_POINT_SIZE);
    glUniformMatrix4fv(lightShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(m_VAO);
    glDrawArrays(GL_POINTS, 0, m_lights.size());
}

void LightManager::drawShadowMaps(
    const Shader &shadowShader, 
    const glm::mat4 &modelMatrix,
    const glm::mat4 &projectionMatrix,
    std::vector<GPUMesh> &meshes) {
    // update viewport
    glViewport(0, 0, Light::SHADOW_WIDTH, Light::SHADOW_HEIGHT);

    glEnable(GL_DEPTH_TEST);
    // glCullFace(GL_FRONT);

    for (auto &light: m_lights) {
        light.drawShadowMap(shadowShader, projectionMatrix * light.m_camera.viewMatrix() * modelMatrix, meshes);
    }

    // unbind the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // reset everything
    glViewport(0, 0, utils::WIDTH, utils::HEIGHT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glCullFace(GL_BACK);
}

void LightManager::refreshUBOs() {
    int number_of_lights = static_cast<int>(m_lights.size());
    glBindBuffer(GL_UNIFORM_BUFFER, m_UBO);
    //Set buffer size
    glBufferData(GL_UNIFORM_BUFFER, number_of_lights * sizeof(LightShadingData) + 16, NULL, GL_DYNAMIC_DRAW);
    //Number of lights goes into first 4 bytes
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &number_of_lights);

    // Generate the light data (only position and color)
    std::vector<LightShadingData> light_shading_data;
    for (const auto &light: m_lights) {
        light_shading_data.emplace_back(
            glm::vec4(light.m_camera.m_position, 1.0),
            light.m_color,
            light.m_mvp
        );
    }
    //Now the light data
    glBufferSubData(GL_UNIFORM_BUFFER, 16, number_of_lights * sizeof(LightShadingData), light_shading_data.data());
    //Unbind buffer
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void LightManager::refreshVBOs() {
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    std::vector<glm::vec4> vertices;
    for (const auto &light: m_lights) {
        vertices.emplace_back(light.m_camera.m_position, 1.0);
    }

    glBufferData(GL_ARRAY_BUFFER, m_lights.size() * sizeof(glm::vec4), vertices.data(), GL_DYNAMIC_DRAW);
}
