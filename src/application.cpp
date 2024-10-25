// #include "Image.h"
#include "mesh.h"
#include "texture.h"
// Always include window first (because it includes glfw, which includes GL which needs to be included AFTER glew).
// Can't wait for modules to fix this stuff...
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glad/glad.h>
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <imgui/imgui.h>
DISABLE_WARNINGS_POP()
#include <framework/shader.h>
#include <framework/window.h>
#include <functional>
#include <iostream>
#include <vector>
#include <framework/trackball.h>
#include <camera.h>
#include <constants.h>


const float fixedTimeStep = 0.016f; // 60 ticks per sec
float frameTimeAccumulator = 0.0f; // use this to add up skipped timesteps

std::unique_ptr<Trackball> pTrackball;
std::unique_ptr<Camera> pFlyCamera;
std::unique_ptr<Camera> pMinimapCamera;
std::unique_ptr<Camera> pTppCamera;

std::vector<std::string> cameraModes = {"Trackball", "FlyCamera", "Third Person Camera", "MinimapCamera"};
enum class CameraMode
{
    Trackball,
    FlyCamera,
    ThirdPersonCamera,
    MinimapCamera
};


float followDistance = 5.0f;  // Distance behind the character
float heightOffset = 1.5f;    // Height above the character
float sideOffset = 1.5f;      // Offset to the side (positive for right, negative for left)
glm::vec3 characterOffset = glm::vec3(0.f,0.f,0.f);

std::vector<GPUMesh> crosshair_mesh;

float scaleFactor = 0.5f;

float height = 1.0f * scaleFactor;          // Original height
float aspectRatio = 16.0f / 9.0f;           // 16:9 aspect ratio
float width = height * utils::ASPECT_RATIO; // Calculate width based on height
                                            // Scale down the quad by half

// Now update your quad vertices
glm::vec3 quad_first = {1.f, 0.5f, -1.f};
glm::vec3 quad_sec = {1.f + width, 0.5f, -1.f};
glm::vec3 quad_third = {1.f + width, height + 0.5, -1.f};
glm::vec3 quad_fourth = {1.f, height + 0.5, -1.f};

glm::vec3 minimap_lowcolor = {0.f, 0.f, 1.f};
glm::vec3 minimap_highcolor = {1.f, 0.f, 0.f};

float minimap_ortho_height = 25.f;

bool show_map = false;

CameraMode currentCameraMode = CameraMode::FlyCamera;

// UBOs must always use vec4s, vec2s, or scalars, NEVER vec3 
// https://stackoverflow.com/questions/38172696/should-i-ever-use-a-vec3-inside-of-a-uniform-buffer-or-shader-storage-buffer-o
// This is very annoying.
struct Light {
    glm::vec4 position;
    glm::vec4 color;
};

std::vector<Light> lights = {{glm::vec4(3.f, 8.f, -10.f, -0.f), glm::vec4(1.f, 1.f, 1.f, 0.f)}};

int selectedLightIndex = 0;


class Application
{
public:
    Application()
        : m_window("Final Project", glm::ivec2(utils::WIDTH, utils::HEIGHT), OpenGLVersion::GL41), m_texture(RESOURCE_ROOT "resources/pattern.png"), characterTexture(RESOURCE_ROOT "resources/doggos.jpg")
    {
        pTrackball = std::make_unique<Trackball>(&m_window, glm::radians(50.0f));
        pFlyCamera = std::make_unique<Camera>(&m_window, utils::START_POSITION, utils::START_LOOK_AT);
        pMinimapCamera = std::make_unique<Camera>(&m_window, utils::START_POSITION, utils::START_LOOK_AT);
        pTppCamera = std::make_unique<Camera>(&m_window, utils::START_POSITION, utils::START_LOOK_AT);

        m_window.registerKeyCallback([this](int key, int scancode, int action, int mods)
                                     {
            if (action == GLFW_PRESS)
                onKeyPressed(key, mods);
            else if (action == GLFW_RELEASE)
                onKeyReleased(key, mods); });
        m_window.registerMouseMoveCallback(std::bind(&Application::onMouseMove, this, std::placeholders::_1));
        m_window.registerMouseButtonCallback([this](int button, int action, int mods)
                                             {
            if (action == GLFW_PRESS)
                onMouseClicked(button, mods);
            else if (action == GLFW_RELEASE)
                onMouseReleased(button, mods); });

        m_meshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/scene1.obj");

        characterMesh = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/cylinder.obj");

        try
        {
            ShaderBuilder defaultBuilder;
            defaultBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
            std::cout << "Linked shader_vert.glsl to defaultBuilder" << std::endl;
            defaultBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl");
            std::cout << "Linked shader_frag.glsl to defaultBuilder" << std::endl;
            m_defaultShader = defaultBuilder.build();
            std::cout << "Built m_defaultShader" << std::endl;

            ShaderBuilder shadowBuilder;
            shadowBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl");
            std::cout << "Linked shadow_vert.glsl to shadowBuilder" << std::endl;
            shadowBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "Shaders/shadow_frag.glsl");
            std::cout << "Linked shadow_frag.glsl to shadowBuilder" << std::endl;
            m_shadowShader = shadowBuilder.build();
            std::cout << "Built m_shadowShader" << std::endl;

            ShaderBuilder quadBuilder;
            quadBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/quad_vert.glsl");
            std::cout << "Linked quad_vert.glsl to quadBuilder" << std::endl;
            quadBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/quad_frag.glsl");
            std::cout << "Linked quad_frag.glsl to quadBuilder" << std::endl;
            m_quadShader = quadBuilder.build();
            std::cout << "Built m_quadShader" << std::endl;

            ShaderBuilder minimapBuilder;
            minimapBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/minimap_vert.glsl");
            std::cout << "Linked minimap_vert.glsl to minimapBuilder" << std::endl;
            minimapBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/minimap_frag.glsl");
            std::cout << "Linked minimap_frag.glsl to minimapBuilder" << std::endl;
            m_minimapShader = minimapBuilder.build();
            std::cout << "Built m_minimapShader" << std::endl;

            // Any new shaders can be added below in similar fashion.
            // ==> Don't forget to reconfigure CMake when you do!
            //     Visual Studio: PROJECT => Generate Cache for ComputerGraphics
            //     VS Code: ctrl + shift + p => CMake: Configure => enter
            // ....
        }
        catch (ShaderLoadingException e)
        {
            std::cerr << e.what() << std::endl;
        }
    }

    void update()
    {

        // MINIMAP INITs ***********************************************************************************************
        unsigned int minimapFBO;
        glGenFramebuffers(1, &minimapFBO);

        const int minimapWidth = utils::WIDTH, minimapHeight = utils::HEIGHT;
        unsigned int minimapTex;
        glGenTextures(1, &minimapTex);
        glBindTexture(GL_TEXTURE_2D, minimapTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, minimapWidth, minimapHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);      // Prevents shadow map artifacts.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);      // Prevents shadow map artifacts.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); // Prevents shadow map artifacts.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); // Prevents shadow map artifacts.
        float borderColor[] = {1.0, 1.0, 1.0, 1.0};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor); // Prevents shadow map artifacts.
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, minimapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, minimapTex, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        Texture minimapOverlay = Texture(RESOURCE_ROOT "resources/map_overlay.png");

        GLuint quad_vbo;
        glGenBuffers(1, &quad_vbo);
        GLuint tex_vbo;
        glGenBuffers(1, &tex_vbo);
        GLuint quad_vao;
        glGenVertexArrays(1, &quad_vao);
        GLuint quad_ibo;
        glGenBuffers(1, &quad_ibo);

        // END MINIMAP INITs ********************************************************************************************

        // LIGHT UBO ****************************************************************************************************
        int number_of_lights = (int) lights.size();

        GLuint lightUBO;
        glGenBuffers(1, &lightUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, lightUBO);
        //Set buffer size
        glBufferData(GL_UNIFORM_BUFFER, number_of_lights * sizeof(Light) + 16, NULL, GL_DYNAMIC_DRAW);
        //Number of lights goes into first 4 bytes
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &number_of_lights);
        //Now the light data
        glBufferSubData(GL_UNIFORM_BUFFER, 16, number_of_lights * sizeof(Light), lights.data());
        //Unbind buffer
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // For updating lights
        auto refreshLightsUBO = [&] {
            int number_of_lights = (int) lights.size();
             glBindBuffer(GL_UNIFORM_BUFFER, lightUBO);
            //Set buffer size
            glBufferData(GL_UNIFORM_BUFFER, number_of_lights * sizeof(Light) + 16, NULL, GL_DYNAMIC_DRAW);
            //Number of lights goes into first 4 bytes
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &number_of_lights);
            //Now the light data
            glBufferSubData(GL_UNIFORM_BUFFER, 16, number_of_lights * sizeof(Light), lights.data());
            //Unbind buffer
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        };
        // END LIGHT UBO ************************************************************************************************
        // RENDER FUNCTIONS *********************************************************************************************
        auto renderMinimapTexture = [&](Shader &shader)
        {
            glEnable(GL_DEPTH_TEST);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

            // Bind off-screen framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, minimapFBO);

            // m_viewMatrix = pMinimapCamera->viewMatrix();
            // TODO: This should be changed to an actual function in camera.cpp
            float orthoWidth = minimap_ortho_height * utils::ASPECT_RATIO;

            const glm::mat4 m_projection2 = glm::ortho(-orthoWidth, orthoWidth, -minimap_ortho_height, minimap_ortho_height, 0.1f, 100.0f);

             for (GPUMesh &mesh : m_meshes)
            {
                const glm::mat4 mvpMatrix = m_projection2 * pMinimapCamera->viewMatrix() * mesh.modelMatrix;
                // Normals should be transformed differently than positions (ignoring translations + dealing with scaling):
                // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
                const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(mesh.modelMatrix));
                shader.bind();
                //!! IMPORTANT -> mesh.draw binds material to block 0, we bind lightBuffer to 1 instead.
                shader.bindUniformBlock("lightBuffer", 1, lightUBO);
                glUniform3fv(shader.getUniformLocation("cameraPosition"), 1, glm::value_ptr(pFlyCamera->cameraPos()));
                glUniformMatrix4fv(shader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
                // Uncomment this line when you use the modelMatrix (or fragmentPosition)
                // glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
                glUniformMatrix3fv(shader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
                if (mesh.hasTextureCoords())
                {
                    m_texture.bind(GL_TEXTURE0);
                    glUniform1i(shader.getUniformLocation("colorMap"), 0);
                    glUniform1i(shader.getUniformLocation("hasTexCoords"), GL_TRUE);
                    glUniform1i(shader.getUniformLocation("useMaterial"), GL_FALSE);
                }
                else
                {
                    glUniform1i(shader.getUniformLocation("hasTexCoords"), GL_FALSE);
                    glUniform1i(shader.getUniformLocation("useMaterial"), m_useMaterial);
                }
                mesh.draw(shader);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        };
        auto renderMinimap = [&]
        {
            glDisable(GL_DEPTH_TEST);
            m_quadShader.bind();
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, minimapTex);
            glUniform1i(m_quadShader.getUniformLocation("texture1"), 2);
            minimapOverlay.bind(GL_TEXTURE1);
            glUniform1i(m_quadShader.getUniformLocation("overlay"), 1);

            const glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix * m_modelMatrix;
            // Normals should be transformed differently than positions (ignoring translations + dealing with scaling):
            // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
            const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(m_modelMatrix));

            glUniformMatrix4fv(m_quadShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));

            // Create Quad covering half screen
            //  Define quad vertices and indices
            int quad_indices[6] = {0, 1, 2, 3, 0, 2};
            // glm::vec3 quadFirst  = { 0.3f, 0.3f, -1.f };
            // glm::vec3 quadSec    = { 1.0f, 0.3f, -1.f };
            // glm::vec3 quadThird  = { 1.0f, 1.0f, -1.f };
            // glm::vec3 quadFourth = { 0.3f, 1.0f, -1.f };
            glm::vec3 quad_vertices[4] = {
                quad_first, quad_sec, quad_third, quad_fourth};

            // Transform quad vertices with the view matrix
            for (auto &vertex : quad_vertices)
            {
                glm::vec4 transformed_vertex = glm::inverse(pFlyCamera->viewMatrix()) * glm::vec4(vertex, 1.0f);
                vertex = glm::vec3(transformed_vertex);
            }

            glm::vec2 quad_tex_coords[4] = {
                {0.0f, 0.0f}, // Texture coordinate for quadFirst
                {1.0f, 0.0f}, // Texture coordinate for quadSec
                {1.0f, 1.0f}, // Texture coordinate for quadThird
                {0.0f, 1.0f}  // Texture coordinate for quadFourth
            };

            // Create and bind the vertex buffer object (VBO) for positions
            
            glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
            glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec3), quad_vertices, GL_STATIC_DRAW);

            // Create and bind the VBO for texture coordinates
            glBindBuffer(GL_ARRAY_BUFFER, tex_vbo);
            glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec2), quad_tex_coords, GL_STATIC_DRAW);

            // Set up the vertex array object (VAO)
            glBindVertexArray(quad_vao);

            // Bind the vertex position VBO
            glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
            glEnableVertexAttribArray(0); // Layout location 0 (position)
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);

            // Bind the texture coordinate VBO
            glBindBuffer(GL_ARRAY_BUFFER, tex_vbo);
            glEnableVertexAttribArray(1); // Layout location 1 (texture coordinates)
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void *)0);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_ibo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(int), quad_indices, GL_STATIC_DRAW);
            // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Do not unbind the element array buffer

            // Bind the index buffer object (IBO)
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_ibo);

            // Drawing
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glBindVertexArray(0);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        };

        auto renderScene = [&](const Shader &shader)
        {
            for (GPUMesh &mesh : m_meshes)
            {
                const glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix * mesh.modelMatrix;
                // Normals should be transformed differently than positions (ignoring translations + dealing with scaling):
                // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
                const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(mesh.modelMatrix));
                shader.bind();
                //!! IMPORTANT -> mesh.draw binds material to block 0, we bind lightBuffer to 1 instead.
                shader.bindUniformBlock("lightBuffer", 1, lightUBO);
                glUniform3fv(shader.getUniformLocation("cameraPosition"), 1, glm::value_ptr(pFlyCamera->cameraPos()));
                glUniformMatrix4fv(shader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
                // Uncomment this line when you use the modelMatrix (or fragmentPosition)
                // glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
                glUniformMatrix3fv(shader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
                if (mesh.hasTextureCoords())
                {
                    m_texture.bind(GL_TEXTURE0);
                    glUniform1i(shader.getUniformLocation("colorMap"), 0);
                    glUniform1i(shader.getUniformLocation("hasTexCoords"), GL_TRUE);
                    glUniform1i(shader.getUniformLocation("useMaterial"), GL_FALSE);
                }
                else
                {
                    glUniform1i(shader.getUniformLocation("hasTexCoords"), GL_FALSE);
                    glUniform1i(shader.getUniformLocation("useMaterial"), m_useMaterial);
                }
                mesh.draw(shader);
            }
        };

        auto renderMeshes = [&](const Shader &shader, std::vector<GPUMesh> &meshes, Texture &texture)
        {
            for (GPUMesh &mesh : meshes)
            {
                const glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix * mesh.modelMatrix;
                // Normals should be transformed differently than positions (ignoring translations + dealing with scaling):
                // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
                const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(mesh.modelMatrix));
                shader.bind();
                //!! IMPORTANT -> mesh.draw binds material to block 0, we bind lightBuffer to 1 instead.
                shader.bindUniformBlock("lightBuffer", 1, lightUBO);
                glUniform3fv(shader.getUniformLocation("cameraPosition"), 1, glm::value_ptr(pFlyCamera->cameraPos()));
                glUniformMatrix4fv(shader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
                // Uncomment this line when you use the modelMatrix (or fragmentPosition)
                // glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
                glUniformMatrix3fv(shader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
                if (mesh.hasTextureCoords())
                {
                    texture.bind(GL_TEXTURE0);
                    glUniform1i(shader.getUniformLocation("colorMap"), 0);
                    glUniform1i(shader.getUniformLocation("hasTexCoords"), GL_TRUE);
                    glUniform1i(shader.getUniformLocation("useMaterial"), GL_FALSE);
                }
                else
                {
                    glUniform1i(shader.getUniformLocation("hasTexCoords"), GL_FALSE);
                    glUniform1i(shader.getUniformLocation("useMaterial"), m_useMaterial);
                }
                mesh.draw(shader);
            }
        };
        // GAME LOOP ****************************************************************************************************

        std::vector<GPUMesh> fireMesh = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/fireframes/firecube.obj");
        std::vector<Texture> fireTextures;
        fireTextures.push_back(Texture(RESOURCE_ROOT "resources/fireframes/frame1.png"));
        fireTextures.push_back(Texture(RESOURCE_ROOT "resources/fireframes/frame2.png"));
        fireTextures.push_back(Texture(RESOURCE_ROOT "resources/fireframes/frame3.png"));

        Texture* activeFireTexture = &fireTextures[0];
        int frameCounter = 0;

        float previousTime = static_cast<float>(glfwGetTime());
        while (!m_window.shouldClose())
        {
            // This is your game loop
            // Put your real-time logic and rendering in here

            float currentTime = static_cast<float>(glfwGetTime());
            float frameTime = currentTime - previousTime;
            previousTime = currentTime;
            frameTimeAccumulator += frameTime;

            while(frameTimeAccumulator >= fixedTimeStep) {

                frameCounter++;
                activeFireTexture = &fireTextures[((int) frameCounter/20) % fireTextures.size()];
                frameTimeAccumulator -= fixedTimeStep;
            }

            ImGuiIO& io = ImGui::GetIO();

            m_window.updateInput();
            switch (currentCameraMode) {
                case CameraMode::FlyCamera:
                case CameraMode::MinimapCamera:
                case CameraMode::ThirdPersonCamera:
                    if (!io.WantCaptureMouse) {
                        pFlyCamera->updateInput();
                    }
                    break;
                default:
                    break;
            }

            { // Use ImGui for easy input/output of ints, floats, strings, etc...
                ImGui::Begin("Window");
                ImGui::Checkbox("Use material if no texture", &m_useMaterial);
                ImGui::Text("Camera Mode");
                if (ImGui::BeginCombo("##combo", cameraModes[static_cast<int>(currentCameraMode)].c_str()))
                {
                    for (unsigned int n = 0; n < cameraModes.size(); n++)
                    {
                        bool isSelected = (currentCameraMode == static_cast<CameraMode>(n));
                        if (ImGui::Selectable(cameraModes[n].c_str(), isSelected))
                        {
                            currentCameraMode = static_cast<CameraMode>(n);
                        }
                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                if (ImGui::Button("Reset FlyCamera"))
                {
                    pFlyCamera->m_position = utils::START_POSITION;
                    pFlyCamera->m_forward = glm::normalize(utils::START_LOOK_AT - utils::START_POSITION);
                }

                if (ImGui::CollapsingHeader("Third Person Camera"))
                {
                    ImGui::DragFloat("Follow Distance", &followDistance, 0.1f, 0.0f, 20.0f, "%.1f");
                    ImGui::DragFloat("Height Offset", &heightOffset, 0.1f, 0.0f, 20.0f, "%.1f");
                    ImGui::DragFloat("Side Offset", &sideOffset, 0.1f, -10.0f, 10.0f, "%.1f");
                    ImGui::DragFloat3("Character Offset", glm::value_ptr(characterOffset), 0.1f, -10.0f, 10.0f, "%.1f");
                }

                if (ImGui::CollapsingHeader("FlyCamera Info"))
                {
                    ImGui::Text("FlyCamera Position: (%.2f, %.2f, %.2f)", pFlyCamera->m_position.x, pFlyCamera->m_position.y, pFlyCamera->m_position.z);
                    ImGui::Text("FlyCamera Forward: (%.2f, %.2f, %.2f)", pFlyCamera->m_forward.x, pFlyCamera->m_forward.y, pFlyCamera->m_forward.z);
                    ImGui::Text("FlyCamera Up: (%.2f, %.2f, %.2f)", pFlyCamera->m_up.x, pFlyCamera->m_up.y, pFlyCamera->m_up.z);
                }

                ImGui::Separator();
                if (ImGui::CollapsingHeader("Minimap"))
                {
                    ImGui::DragFloat("Ortho Height", &minimap_ortho_height, 0.1f, 1.0f, 80.0f, "%.1f");
                    if (ImGui::CollapsingHeader("Minimap Position"))
                    {
                        ImGui::DragFloat3("Quad First", glm::value_ptr(quad_first), 0.01f, -1.0f, 1.8f, "%.2f");
                        ImGui::DragFloat3("Quad Second", glm::value_ptr(quad_sec), 0.01f, -1.0f, 1.8f, "%.2f");
                        ImGui::DragFloat3("Quad Third", glm::value_ptr(quad_third), 0.01f, -1.0f, 1.8f, "%.2f");
                        ImGui::DragFloat3("Quad Fourth", glm::value_ptr(quad_fourth), 0.01f, -1.0f, 1.8f, "%.2f");
                    }
                }

                if (ImGui::CollapsingHeader("Lights"))
                {
                    // Display lights in scene
                    std::vector<std::string> itemStrings = {};
                    for (size_t i = 0; i < lights.size(); i++)
                    {
                        auto string = "Light " + std::to_string(i) + "|R " + std::to_string(lights[i].color.r).substr(0, 5) + " |G" + std::to_string(lights[i].color.g).substr(0, 5) + " |B " + std::to_string(lights[i].color.b).substr(0, 5);
                        itemStrings.push_back(string);
                    }

                    std::vector<const char *> itemCStrings = {};
                    for (const auto &string : itemStrings)
                    {
                        itemCStrings.push_back(string.c_str());
                    }

                    int tempSelectedItem = static_cast<int>(selectedLightIndex);
                    if (ImGui::ListBox("Lights", &tempSelectedItem, itemCStrings.data(), (int)itemCStrings.size(), 4))
                    {
                        selectedLightIndex = static_cast<size_t>(tempSelectedItem);
                    }

                    if (ImGui::Button("Add Light"))
                    {
                        lights.push_back(Light{glm::vec4(0, 0, 3, 0.f), glm::vec4(1)});
                        selectedLightIndex = lights.size() - 1;
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Remove Light"))
                    {
                        if (lights.size() > 1)
                        {
                            lights.erase(lights.begin() + selectedLightIndex);
                            selectedLightIndex = 0;
                        }
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Move Light to Camera")) {
                        lights[selectedLightIndex].position = glm::vec4(pFlyCamera->m_position, 1.0f);
                    }

                    // Slider for selected camera pos
                    ImGui::DragFloat4("Position", glm::value_ptr(lights[selectedLightIndex].position), 0.1f, -10.0f, 10.0f);

                    // Color picker for selected light
                    ImGui::ColorEdit4("Color", &lights[selectedLightIndex].color[0]);
                    refreshLightsUBO();
                }
                ImGui::End();
            }

            // Clear the screen
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // ...
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);

            // TODO: We should change this to be actual character controls, but I hate the idea of it.
            switch (currentCameraMode)
            {
            case CameraMode::Trackball:
                pTrackball->enableTranslation();
                m_viewMatrix = pTrackball->viewMatrix();
                m_projectionMatrix = pTrackball->projectionMatrix();
                break;
            case CameraMode::FlyCamera:
                pTrackball->disableTranslation();
                m_viewMatrix = pFlyCamera->viewMatrix();
                // TODO: This should be changed to an actual function in camera.cpp
                m_projectionMatrix = glm::perspective(utils::FOV, m_window.getAspectRatio(), 0.1f, 100.0f);
                break;
            case CameraMode::ThirdPersonCamera:
                pTrackball->disableTranslation();
                m_viewMatrix = pTppCamera->viewMatrix();
                // TODO: This should be changed to an actual function in camera.cpp
                m_projectionMatrix = glm::perspective(utils::FOV, m_window.getAspectRatio(), 0.1f, 100.0f);
                break;
            case CameraMode::MinimapCamera:

                m_viewMatrix = pMinimapCamera->viewMatrix();
                // TODO: This should be changed to an actual function in camera.cpp
                float orthoWidth = minimap_ortho_height * utils::ASPECT_RATIO;

                const glm::mat4 m_projection2 = glm::ortho(-orthoWidth, orthoWidth, -minimap_ortho_height, minimap_ortho_height, 0.1f, 100.0f);
                m_projectionMatrix = m_projection2;
            }

            { // MINIMAP
                pMinimapCamera->m_position = pFlyCamera->m_position;
                pMinimapCamera->m_forward = glm::vec3(0.f, -1.f, 0.f);
                pMinimapCamera->m_up = glm::vec3(pFlyCamera->m_forward.x, 0.f, pFlyCamera->m_forward.z);
            }

            if(currentCameraMode == CameraMode::ThirdPersonCamera){
                // Calculate a rightward direction from the character's forward vector
                glm::vec3 rightOffset = glm::normalize(glm::cross(pFlyCamera->m_forward, glm::vec3(0.0f, 1.0f, 0.0f))) * sideOffset;

                // Set pTppCamera position relative to the character with an additional side offset
                pTppCamera->m_position = pFlyCamera->m_position 
                                        - followDistance * pFlyCamera->m_forward 
                                        + glm::vec3(0.0f, heightOffset, 0.0f)
                                        + rightOffset;

                // Set the forward direction of the camera to look at the character's position
                pTppCamera->m_forward = pFlyCamera->m_forward;

                // Calculate a right vector based on the camera's forward direction and world up vector
                glm::vec3 rightVector = glm::normalize(glm::cross(pTppCamera->m_forward, glm::vec3(0.0f, 1.0f, 0.0f)));

                // Recalculate up vector as perpendicular to forward and right, ensuring it’s stable even when looking up/down
                pTppCamera->m_up = glm::cross(rightVector, pTppCamera->m_forward);

                
                renderMeshes(m_defaultShader, characterMesh, characterTexture);

            }

            for(GPUMesh &mesh : characterMesh) {
                mesh.attachToCamera(pFlyCamera->m_position, pFlyCamera->m_forward, pFlyCamera->m_up, characterOffset);
            }

            if(show_map) renderMinimapTexture(m_defaultShader);

            renderScene(m_defaultShader);

            renderMeshes(m_defaultShader, fireMesh, *activeFireTexture);

            // render quad

            if(show_map) renderMinimap();
            // Processes input and swaps the window buffer
            m_window.swapBuffers();
        }
    }

    // In here you can handle key presses
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyPressed(int key, int mods)
    {

        if(key == GLFW_KEY_M) show_map = !show_map;;

        if(key == GLFW_KEY_V) {
            if (currentCameraMode == CameraMode::FlyCamera) {
                currentCameraMode = CameraMode::ThirdPersonCamera;
            } else if (currentCameraMode == CameraMode::ThirdPersonCamera) {
                currentCameraMode = CameraMode::FlyCamera;
            }
        }
        std::cout << "Key pressed: " << key << std::endl;
    }

    // In here you can handle key releases
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyReleased(int key, int mods)
    {
        std::cout << "Key released: " << key << std::endl;
    }

    // If the mouse is moved this function will be called with the x, y screen-coordinates of the mouse
    void onMouseMove(const glm::dvec2 &cursorPos)
    {
        std::cout << "Mouse at position: " << cursorPos.x << " " << cursorPos.y << std::endl;
    }

    // If one of the mouse buttons is pressed this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseClicked(int button, int mods)
    {
        std::cout << "Pressed mouse button: " << button << std::endl;
    }

    // If one of the mouse buttons is released this function will be called
    // button - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__buttons.html
    // mods - Any modifier buttons pressed
    void onMouseReleased(int button, int mods)
    {
        std::cout << "Released mouse button: " << button << std::endl;
    }

private:
    Window m_window;

    // Shader for default rendering and for depth rendering
    Shader m_defaultShader;
    Shader m_shadowShader;
    Shader m_quadShader;
    Shader m_minimapShader;

    std::vector<GPUMesh> m_meshes;
    std::vector<GPUMesh> characterMesh;
    Texture m_texture;
    Texture characterTexture;
    bool m_useMaterial{true};

    // Projection and view matrices for you to fill in and use
    glm::mat4 m_projectionMatrix = glm::perspective(glm::radians(80.0f), 1.0f, 0.1f, 30.0f);
    glm::mat4 m_viewMatrix = glm::lookAt(glm::vec3(-1, 1, -1), glm::vec3(0), glm::vec3(0, 1, 0));
    glm::mat4 m_modelMatrix{1.0f};
};

int main()
{
    Application app;
    app.update();

    return 0;
}
