#ifndef LIGHTS_H
#define LIGHTS_H
#include <camera.h>
#include <vector>
#include <framework/shader.h>
#include <glm/vec4.hpp>


namespace lum {
    struct LightShadingData {
        glm::vec4 position;
        glm::vec4 color;
    };
    
    class Light {
    public:
        Light(Window* window, glm::vec3 position, glm::vec4 color);
    
        glm::mat4 lightMatrix();

        std::string toString() const;

    public:
        Camera m_camera;
        glm::vec4 m_color;
    };

    
    class LightManager {
    public:
        explicit LightManager(std::vector<Light> lights);

        Light& crtLight();
        void drawLights(const Shader &lightShader, const glm::mat4& mvp);
        // void addLight(Window* window, glm::vec3 position, glm::vec4 color);
        // void removeCrtLight();
        
        void refreshUBOs();
        void refreshVBOs();
        void generateVAOs();

    public: 
        std::vector<Light> m_lights;
        GLuint m_UBO;
        GLuint m_VBO;
        GLuint m_VAO;
    
        unsigned int m_selectedLightIndex = 0;
    };
}


#endif //LIGHTS_H
