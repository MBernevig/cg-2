#pragma once
#include <camera.h>
#include <vector>
#include <framework/mesh.h>
#include <framework/shader.h>
#include <glm/vec4.hpp>

#include "../mesh.h"


namespace lum {
    struct LightShadingData {
        glm::vec4 position;
        glm::vec4 color;
        glm::mat4 lightMVP;
    };
    
    class Light {
    public:
        Light(Window* window, glm::vec3 position, glm::vec4 color);
        std::string toString() const;
        void drawShadowMap(const Shader &shadowShader, const glm::mat4 & mvpMatrix, std::vector<GPUMesh> &meshes);

    public:
        constexpr static int SHADOW_HEIGHT = 1024;
        constexpr static int SHADOW_WIDTH = 1024;
        
        Camera m_camera;
        glm::vec4 m_color;

        glm::mat4 m_mvp = glm::mat4(1.f);

        GLuint m_shadowMapFBO;
        GLuint m_shadowMap;
    };

    
    class LightManager {
    public:
        explicit LightManager(std::vector<Light> lights);

        Light& crtLight();
        void drawLights(const Shader &lightShader, const glm::mat4& mvp);

        void drawShadowMaps(const Shader &shadowShader, const glm::mat4 &modelMatrix,
                            const glm::mat4 &projectionMatrix, std::vector<GPUMesh> &meshes);
        
        void refreshUBOs();
        void refreshVBOs();

    public: 
        std::vector<Light> m_lights;
        GLuint m_UBO{};
        GLuint m_VBO{};
        GLuint m_VAO{};
        GLuint m_shadowMapArray;
    
        unsigned int m_selectedLightIndex = 0;
    };
}
