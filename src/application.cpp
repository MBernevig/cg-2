// #include "Image.h"
#include "mesh.h"
#include "texture.h"
// Always include window first (because it includes glfw, which includes GL which needs to be included AFTER glew).
// Can't wait for modules to fix this stuff...
#include <framework/disable_all_warnings.h>

#include "BezierCurve.h"
#include "lights/lights.h"
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
#include <stb/stb_image.h>
#include <camera.h>
#include <constants.h>

float skyboxVertices[] =
    {
        //   Coordinates
        -1.0f, -1.0f, 1.0f,  //        7--------6
        1.0f, -1.0f, 1.0f,   //       /|       /|
        1.0f, -1.0f, -1.0f,  //      4--------5 |
        -1.0f, -1.0f, -1.0f, //      | |      | |
        -1.0f, 1.0f, 1.0f,   //      | 3------|-2
        1.0f, 1.0f, 1.0f,    //      |/       |/
        1.0f, 1.0f, -1.0f,   //      0--------1
        -1.0f, 1.0f, -1.0f};

unsigned int skyboxIndices[] =
    {
        // Right
        1, 2, 6,
        6, 5, 1,
        // Left
        0, 4, 7,
        7, 3, 0,
        // Top
        4, 5, 6,
        6, 7, 4,
        // Bottom
        0, 3, 2,
        2, 1, 0,
        // Back
        0, 1, 5,
        5, 4, 0,
        // Front
        3, 7, 6,
        6, 2, 3};

const float fixedTimeStep = 0.016f; // 60 ticks per sec
float frameTimeAccumulator = 0.0f;  // use this to add up skipped timesteps

bool showMinimap = true;
bool renderMainScene = true;
bool drawCube = true;
bool drawCurve = true;
bool bezierLight = true;
bool reflectMode = true;
float refractionIndex = 1.0f;
float cubeRotation = 0.5f;
int shadowMode = 1;
int pcfSampleCount = 4;

std::unique_ptr<Camera> pFlyCamera;
std::unique_ptr<Camera> pMinimapCamera;
std::unique_ptr<Camera> pTppCamera;

std::vector<std::string> cameraModes = {"FlyCamera", "ThirdPersonCamera", "MinimapCamera", "LightCamera"};
enum class CameraMode
{
    FlyCamera,
    ThirdPersonCamera,
    MinimapCamera,
    LightCamera
};

float followDistance = 5.0f; // Distance behind the character
float heightOffset = 1.5f;   // Height above the character
float sideOffset = 1.5f;     // Offset to the side (positive for right, negative for left)
glm::vec3 characterOffset = glm::vec3(0.f, -3.5f, 0.f);

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

CameraMode currentCameraMode = CameraMode::FlyCamera;

// UBOs must always use vec4s, vec2s, or scalars, NEVER vec3
// https://stackoverflow.com/questions/38172696/should-i-ever-use-a-vec3-inside-of-a-uniform-buffer-or-shader-storage-buffer-o
// This is very annoying.
struct Light
{
    glm::vec4 position;
    glm::vec4 color;
};

class Application
{
public:
    Application()
    : m_window("Final Project", glm::ivec2(utils::WIDTH, utils::HEIGHT), OpenGLVersion::GL41),
    m_texture(RESOURCE_ROOT "resources/brickwall.jpg"),
    m_normalMap(RESOURCE_ROOT "resources/brickwall_normal.jpg"),
    m_lightManager({
        lum::Light(&m_window, glm::vec4(6.f, 3.f, -10.f, -0.f), glm::vec4(1.f, 1.f, 1.f, 0.f))
    }),
    m_curve({
        {40.0, 40.0, 40.0},
        {50.0, 20.0, 40.0},
        {40.0, 50.0, -60.0},
        {70.0, 40.0, -40.0},
    }){
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

        m_meshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/scene2.obj");

        try
        {
            ShaderBuilder defaultBuilder;
            defaultBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
            defaultBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl");
            m_defaultShader = defaultBuilder.build();

            ShaderBuilder shadowBuilder;
            shadowBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl");
            shadowBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "Shaders/shadow_frag.glsl");
            m_shadowShader = shadowBuilder.build();

            ShaderBuilder quadBuilder;
            quadBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/quad_vert.glsl");
            quadBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/quad_frag.glsl");
            m_quadShader = quadBuilder.build();

            ShaderBuilder minimapBuilder;
            minimapBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/minimap_vert.glsl");
            minimapBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/minimap_frag.glsl");
            m_minimapShader = minimapBuilder.build();

            ShaderBuilder lightBuilder;
            lightBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/light_vertex.glsl");
            lightBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/light_frag.glsl");
            m_lightShader = lightBuilder.build();

            ShaderBuilder skyboxBuilder;
            skyboxBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/skybox_vert.glsl");
            skyboxBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/skybox_frag.glsl");
            m_skyboxShader = skyboxBuilder.build();

            ShaderBuilder cubeBuilder;
            cubeBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/cube_vert.glsl");
            cubeBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/cube_frag.glsl");
            m_cubeShader = cubeBuilder.build();

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

        // face culling for better shadows
        glEnable(GL_CULL_FACE);
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

        GLuint quad_vbo;
        glGenBuffers(1, &quad_vbo);
        GLuint tex_vbo;
        glGenBuffers(1, &tex_vbo);
        GLuint quad_vao;
        glGenVertexArrays(1, &quad_vao);
        GLuint quad_ibo;
        glGenBuffers(1, &quad_ibo);

        // END MINIMAP INITs ********************************************************************************************

        // EXTRA MESHES

        std::vector<GPUMesh> cube = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/unitCube.obj");

        std::string facesSkybox[6] = {
            RESOURCE_ROOT "resources/textures/px.png",
            RESOURCE_ROOT "resources/textures/nx.png",
            RESOURCE_ROOT "resources/textures/py.png",
            RESOURCE_ROOT "resources/textures/ny.png",
            RESOURCE_ROOT "resources/textures/pz.png",
            RESOURCE_ROOT "resources/textures/nz.png"};

        unsigned int skyboxTex;
        glGenTextures(1, &skyboxTex);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);

        for (unsigned int i = 0; i < 6; i++)
        {
            int widthSky, heightSky, nrChannels;
            unsigned char *data = stbi_load(facesSkybox[i].c_str(), &widthSky, &heightSky, &nrChannels, 0);
            if (data)
            {
                glTexImage2D(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                    0,
                    GL_RGBA,
                    widthSky,
                    heightSky,
                    0,
                    GL_RGBA,
                    GL_UNSIGNED_BYTE,
                    data);
                stbi_image_free(data);
            }
            else
            {
                std::cout << "Cubemap texture failed to load at path: " << facesSkybox[i] << std::endl;
                stbi_image_free(data);
            }
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        std::vector<GPUMesh> screen = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/unitCube.obj");

        std::vector<GPUMesh> character = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/character.obj");

        // Add screen to m_meshes and keep a pointer to the added value
        Texture screenTexture(RESOURCE_ROOT "resources/textures/doggos.jpg");

        // screen[0].texture = &screenTexture;
        // screenMesh->texture = &screenTexture;

        m_meshes.emplace_back(std::move(character[0]));
        GPUMesh *characterMesh = &m_meshes.back();
        characterMesh->renderFPV = false;

        // Create VAO, VBO, and EBO for the skybox
        unsigned int skyboxVAO, skyboxVBO, skyboxEBO;
        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);
        glGenBuffers(1, &skyboxEBO);
        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), &skyboxIndices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // RENDER FUNCTIONS *********************************************************************************************
        auto renderCube = [&] {
            m_cubeShader.bind();
            const glm::mat4 mvpMatrix = m_projectionMatrix * pFlyCamera->viewMatrix() * cube[0].modelMatrix;
                // Normals should be transformed differently than positions (ignoring translations + dealing with scaling):
                // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
            const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(cube[0].modelMatrix));

            glUniform3fv(m_cubeShader.getUniformLocation("cameraPosition"), 1, glm::value_ptr(pFlyCamera->cameraPos()));
            glUniformMatrix4fv(m_cubeShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
            glUniformMatrix4fv(m_cubeShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(cube[0].modelMatrix));
            glUniformMatrix3fv(m_cubeShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);
            glUniform1i(m_cubeShader.getUniformLocation("skybox"), 2);

            glUniform1i(m_cubeShader.getUniformLocation("reflectMode"), (reflectMode? 1 : 0));
            glUniform1f(m_cubeShader.getUniformLocation("refractionIndex"), refractionIndex);

            cube[0].draw(m_cubeShader);

        };

        auto renderSkybox = [&]
        {
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glDepthFunc(GL_LEQUAL);

            m_skyboxShader.bind();
            glm::mat4 view = glm::mat4(1.0f);
            // We make the mat4 into a mat3 and then a mat4 again in order to get rid of the last row and column
            // The last row and column affect the translation of the skybox (which we don't want to affect)
            view = glm::mat4(glm::mat3(m_viewMatrix));
            glUniformMatrix4fv(m_skyboxShader.getUniformLocation("viewSkybox"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(m_skyboxShader.getUniformLocation("projectionSkybox"), 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));

            // Draws the cubemap as the last object so we can save a bit of performance by discarding all fragments
            // where an object is present (a depth of 1.0f will always fail against any object's depth value)
            // glDepthMask(GL_FALSE);
            glBindVertexArray(skyboxVAO);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);
            glUniform1i(m_skyboxShader.getUniformLocation("skybox"), 2);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            // glDepthMask(GL_TRUE);

            // Switch back to the normal depth function
            glDepthFunc(GL_LESS);
            glEnable(GL_CULL_FACE);
        };

        auto renderMinimapTexture = [&]
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
                m_defaultShader.bind();
                //!! IMPORTANT -> mesh.draw binds material to block 0, we bind lightBuffer to 1 instead.
                m_defaultShader.bindUniformBlock("lightBuffer", 1, m_lightManager.m_UBO);
                glUniform3fv(m_defaultShader.getUniformLocation("cameraPosition"), 1, glm::value_ptr(pFlyCamera->cameraPos()));
                glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
                glUniformMatrix4fv(m_defaultShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(mesh.modelMatrix));
                glUniformMatrix3fv(m_defaultShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
                glUniform1i(m_defaultShader.getUniformLocation("useNormalMap"), mesh.normalMap != nullptr);

                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);
                glUniform1i(m_defaultShader.getUniformLocation("skybox"), 2);
                if (mesh.hasTextureCoords())
                {
                    if(mesh.texture) mesh.texture->bind(GL_TEXTURE0); else m_texture.bind(GL_TEXTURE0);
                    glUniform1i(m_defaultShader.getUniformLocation("colorMap"), 0);
                    glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_TRUE);
                    glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), GL_FALSE);

                    if (mesh.normalMap != nullptr) {
                        mesh.normalMap->bind(GL_TEXTURE1);
                        glUniform1i(m_defaultShader.getUniformLocation("normalMap"), 1);
                    }
                }
                else
                {
                    glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_FALSE);
                    glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), m_useMaterial);
                }
                // bind shadow textures
                int textureIndices[32];
                for (int i = 0; i < m_lightManager.m_lights.size(); i++)
                {
                    glActiveTexture(GL_TEXTURE3 + i);
                    glBindTexture(GL_TEXTURE_2D, m_lightManager.m_lights[i].m_shadowMap);
                    textureIndices[i] = i + 3;
                }
                glUniform1iv(m_defaultShader.getUniformLocation("shadowMap"), m_lightManager.m_lights.size(), textureIndices);
                glUniform1i(m_defaultShader.getUniformLocation("shadowMode"), shadowMode);
                glUniform1i(m_defaultShader.getUniformLocation("sampleCount"), pcfSampleCount);

                mesh.draw(m_defaultShader);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        };
        auto renderMinimap = [&]
        {
            // glDisable(GL_DEPTH_TEST);
            m_quadShader.bind();
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, minimapTex);
            glUniform1i(m_quadShader.getUniformLocation("texture1"), 2);

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

            glEnable(GL_DEPTH_TEST);
        };

        auto renderScene = [&](const Shader &shader)
        {
            for (GPUMesh &mesh : m_meshes)
            {
                // Don't render character for FPV
                if (currentCameraMode == CameraMode::FlyCamera && !mesh.renderFPV)
                    continue;
                const glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix * mesh.modelMatrix;
                // Normals should be transformed differently than positions (ignoring translations + dealing with scaling):
                // https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
                const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(mesh.modelMatrix));
                shader.bind();
                //!! IMPORTANT -> mesh.draw binds material to block 0, we bind lightBuffer to 1 instead.
                shader.bindUniformBlock("lightBuffer", 1, m_lightManager.m_UBO);
                glUniform3fv(shader.getUniformLocation("cameraPosition"), 1, glm::value_ptr(pFlyCamera->cameraPos()));
                glUniformMatrix4fv(shader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
                glUniformMatrix4fv(shader.getUniformLocation("meshModelMatrix"), 1, GL_FALSE, glm::value_ptr(mesh.modelMatrix));
                // Uncomment this line when you use the modelMatrix (or fragmentPosition)
                glUniformMatrix4fv(shader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(mesh.modelMatrix));
                glUniformMatrix3fv(shader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
                glUniform1i(shader.getUniformLocation("useNormalMap"), mesh.normalMap != nullptr);
                if (mesh.hasTextureCoords())
                {
                    if(mesh.texture) mesh.texture->bind(GL_TEXTURE0); else m_texture.bind(GL_TEXTURE0);
                    glUniform1i(shader.getUniformLocation("colorMap"), 0);
                    glUniform1i(shader.getUniformLocation("hasTexCoords"), GL_TRUE);
                    glUniform1i(shader.getUniformLocation("useMaterial"), GL_FALSE);

                    if (mesh.normalMap != nullptr) {
                        mesh.normalMap->bind(GL_TEXTURE1);
                        glUniform1i(shader.getUniformLocation("normalMap"), 1);
                    }
                }
                else
                {
                    glUniform1i(shader.getUniformLocation("hasTexCoords"), GL_FALSE);
                    glUniform1i(shader.getUniformLocation("useMaterial"), m_useMaterial);
                }
                // bind shadow textures
                int textureIndices[10];
                for (int i = 0; i < m_lightManager.m_lights.size(); i++)
                {
                    glActiveTexture(GL_TEXTURE3 + i);
                    glBindTexture(GL_TEXTURE_2D, m_lightManager.m_lights[i].m_shadowMap);
                    textureIndices[i] = i + 3;
                }
                glUniform1iv(shader.getUniformLocation("shadowMap"), m_lightManager.m_lights.size(), textureIndices);
                glUniform1i(shader.getUniformLocation("shadowMode"), shadowMode);
                glUniform1i(shader.getUniformLocation("sampleCount"), pcfSampleCount);

                mesh.draw(shader);
            }
        };

        int tickCounter = 0;

        float previousTime = static_cast<float>(glfwGetTime());

        // GAME LOOP ****************************************************************************************************
        while (!m_window.shouldClose())
        {

            m_window.updateInput();

            ImGuiIO &io = ImGui::GetIO();

            float currentTime = static_cast<float>(glfwGetTime());
            float frameTime = currentTime - previousTime;
            previousTime = currentTime;
            frameTimeAccumulator += frameTime;

            while (frameTimeAccumulator >= fixedTimeStep)
            {

                tickCounter++;
                cube[0].rotate(glm::radians(cubeRotation), glm::vec3(0,1,0));
                frameTimeAccumulator -= fixedTimeStep;
            }
            switch (currentCameraMode)
            {
            case CameraMode::FlyCamera:
            case CameraMode::MinimapCamera:
            case CameraMode::ThirdPersonCamera:
                if (!io.WantCaptureMouse)
                {
                    pFlyCamera->updateInput();
                }
                break;
            case CameraMode::LightCamera:
                m_lightManager.crtLight().m_camera.updateInput();
                break;
            default:
                break;
            }
            // This is your game loop
            // Put your real-time logic and rendering in here

            imGui();

            m_lightManager.refreshUBOs();

            // TODO: We should change this to be actual character controls, but I hate the idea of it.
            switch (currentCameraMode)
            {
            case CameraMode::FlyCamera:
            {
                m_viewMatrix = pFlyCamera->viewMatrix();
                // TODO: This should be changed to an actual function in camera.cpp
                m_projectionMatrix = glm::perspective(utils::FOV, m_window.getAspectRatio(), 0.1f, 200.0f);
                break;
            }

            case CameraMode::MinimapCamera:
            {
                m_viewMatrix = pMinimapCamera->viewMatrix();
                // TODO: This should be changed to an actual function in camera.cpp
                float orthoWidth = minimap_ortho_height * utils::ASPECT_RATIO;

                const glm::mat4 m_projection2 = glm::ortho(-orthoWidth, orthoWidth, -minimap_ortho_height, minimap_ortho_height, 0.1f, 100.0f);
                m_projectionMatrix = m_projection2;
                break;
            }

            case CameraMode::ThirdPersonCamera:
            {
                m_viewMatrix = pTppCamera->viewMatrix();
                // TODO: This should be changed to an actual function in camera.cpp
                m_projectionMatrix = glm::perspective(utils::FOV, m_window.getAspectRatio(), 0.1f, 200.0f);
                break;
            }

            case CameraMode::LightCamera:
            {
                m_viewMatrix = m_lightManager.crtLight().m_camera.viewMatrix();
                m_projectionMatrix = glm::perspective(utils::FOV, m_window.getAspectRatio(), 0.1f, 200.0f);
                break;
            }
            }

            pMinimapCamera->m_position = pFlyCamera->m_position;
            pMinimapCamera->m_forward = glm::vec3(0.f, -1.f, 0.f);
            glm::vec3 interim = pFlyCamera->m_forward;
            pMinimapCamera->m_up = glm::vec3(interim.x, 0.f, interim.z);

            if (currentCameraMode == CameraMode::ThirdPersonCamera)
            {
                // Calculate a rightward direction from the character's forward vector
                glm::vec3 rightOffset = glm::normalize(glm::cross(pFlyCamera->m_forward, glm::vec3(0.0f, 1.0f, 0.0f))) * sideOffset;

                // Set pTppCamera position relative to the character with an additional side offset
                pTppCamera->m_position = pFlyCamera->m_position - followDistance * pFlyCamera->m_forward + glm::vec3(0.0f, heightOffset, 0.0f) + rightOffset;

                // Set the forward direction of the camera to look at the character's position
                pTppCamera->m_forward = pFlyCamera->m_forward;

                // Calculate a right vector based on the camera's forward direction and world up vector
                glm::vec3 rightVector = glm::normalize(glm::cross(pTppCamera->m_forward, glm::vec3(0.0f, 1.0f, 0.0f)));

                // Recalculate up vector as perpendicular to forward and right, ensuring it’s stable even when looking up/down
                pTppCamera->m_up = glm::cross(rightVector, pTppCamera->m_forward);
            }

            characterMesh->attachToCamera(pFlyCamera->m_position, pFlyCamera->m_forward, pFlyCamera->m_up, characterOffset);
            // Clear the screen
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // ...
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);

            m_lightManager.drawShadowMaps(m_shadowShader, m_modelMatrix, m_projectionMatrix, m_meshes);

            if(renderMainScene) renderScene(m_defaultShader);

            // // render quad
            if (showMinimap)
            {
                renderMinimapTexture();
                renderMinimap();
            }
            // draw the debug lights
            m_lightManager.drawLights(m_lightShader, m_projectionMatrix * m_viewMatrix * m_modelMatrix);

            renderSkybox();

            if(drawCube)
            {
                renderCube();
            }

            if (drawCurve)
            {
                m_curve.drawCurve(m_lightShader, m_projectionMatrix * m_viewMatrix * m_modelMatrix);
            }

            if (bezierLight)
            {
                lum::Light *firstLight = &m_lightManager.m_lights[0];
                firstLight->m_camera.m_position = m_curve.calculateBezierPoint(sin(glfwGetTime() / 2.0f) / 2.0f + 0.5f);
            }
            
            // Processes input and swaps the window buffer
            m_window.swapBuffers();
        }
    }

    void imGui()
    {
        // Use ImGui for easy input/output of ints, floats, strings, etc...
        ImGuiIO &io = ImGui::GetIO();
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
            pFlyCamera->m_forward = normalize(utils::START_LOOK_AT - utils::START_POSITION);
        }

        if (ImGui::CollapsingHeader("FlyCamera Info"))
        {
            ImGui::Text("FlyCamera Position: (%.2f, %.2f, %.2f)", pFlyCamera->m_position.x, pFlyCamera->m_position.y, pFlyCamera->m_position.z);
            ImGui::Text("FlyCamera Forward: (%.2f, %.2f, %.2f)", pFlyCamera->m_forward.x, pFlyCamera->m_forward.y, pFlyCamera->m_forward.z);
            ImGui::Text("FlyCamera Up: (%.2f, %.2f, %.2f)", pFlyCamera->m_up.x, pFlyCamera->m_up.y, pFlyCamera->m_up.z);
        }

        if (ImGui::CollapsingHeader("Third Person Camera"))
        {
            ImGui::DragFloat("Follow Distance", &followDistance, 0.1f, 0.0f, 20.0f, "%.1f");
            ImGui::DragFloat("Height Offset", &heightOffset, 0.1f, 0.0f, 20.0f, "%.1f");
            ImGui::DragFloat("Side Offset", &sideOffset, 0.1f, -10.0f, 10.0f, "%.1f");
            ImGui::DragFloat3("Character Offset", glm::value_ptr(characterOffset), 0.1f, -10.0f, 10.0f, "%.1f");
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

        if (ImGui::CollapsingHeader("Scene Controls"))
        {
            ImGui::Checkbox("Show Minimap", &showMinimap);
            ImGui::Checkbox("Render Main Scene", &renderMainScene);
            ImGui::Checkbox("Draw Cube", &drawCube);
            if(drawCube){
                if (ImGui::CollapsingHeader("Cube Settings"))
                {
                    ImGui::Checkbox("Reflect Mode", &reflectMode);
                    ImGui::DragFloat("Refraction Index", &refractionIndex, 0.01f, 0.0f, 2.0f, "%.2f");
                    ImGui::DragFloat("Cube Rotation", &cubeRotation, 0.1f, 0.0f, 10.0f, "%.1f");
                }
            }
            
            if (ImGui::CollapsingHeader("Bezier Curve Settings"))
            {
                ImGui::Checkbox("Draw Bezier Curve", &drawCurve);
                ImGui::Checkbox("Attach Light", &bezierLight);
                ImGui::DragFloat3("First Control Point", glm::value_ptr(m_curve.m_controlPoints[0]), 5.0f, -100.0f, 100.0f, "%.2f");
                ImGui::DragFloat3("Second Control Point", glm::value_ptr(m_curve.m_controlPoints[1]), 5.0f, -100.0f, 100.0f, "%.2f");
                ImGui::DragFloat3("Third Control Point", glm::value_ptr(m_curve.m_controlPoints[2]), 5.0f, -100.0f, 100.0f, "%.2f");
                ImGui::DragFloat3("Fourth Control Point", glm::value_ptr(m_curve.m_controlPoints[3]), 5.0f, -100.0f, 100.0f, "%.2f");
                if (ImGui::Button("Redraw Curve")) {
                    m_curve.updateCurvePoints();
                }
            }

            ImGui::Text("Shadow Mode");
            ImGui::RadioButton("No Shadows", &shadowMode, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Hard Shadows", &shadowMode, 1);
            ImGui::SameLine();
            ImGui::RadioButton("PCF Shadows", &shadowMode, 2);
            ImGui::Text("0: No shadows, 1: Hard shadows, 2: PCF shadows");
            ImGui::DragInt("PCF Sample Count", &pcfSampleCount, 1, 1, 64);
        }

        if (ImGui::CollapsingHeader("Lights"))
        {
            // Display lights in scene
            std::vector<std::string> itemStrings = {};
            for (size_t i = 0; i < m_lightManager.m_lights.size(); i++)
            {
                auto string = "Light " + std::to_string(i) + m_lightManager.m_lights[i].toString();
                itemStrings.push_back(string);
            }

            std::vector<const char *> itemCStrings = {};
            for (const auto &string : itemStrings)
            {
                itemCStrings.push_back(string.c_str());
            }

            int tempSelectedItem = static_cast<int>(m_lightManager.m_selectedLightIndex);
            if (ImGui::ListBox("Lights", &tempSelectedItem, itemCStrings.data(), (int)itemCStrings.size(), 4))
            {
                m_lightManager.m_selectedLightIndex = static_cast<size_t>(tempSelectedItem);
            }

            if (ImGui::Button("Add Light"))
            {
                m_lightManager.m_lights.emplace_back(&m_window, glm::vec4(0, 0, 3, 0.f), glm::vec4(1));
                m_lightManager.m_selectedLightIndex = m_lightManager.m_lights.size() - 1;
            }

            ImGui::SameLine();
            if (ImGui::Button("Remove Light"))
            {
                if (m_lightManager.m_lights.size() > 1)
                {
                    m_lightManager.m_lights.erase(m_lightManager.m_lights.begin() + m_lightManager.m_selectedLightIndex);
                    m_lightManager.m_selectedLightIndex = 0;
                }
            }

            // Slider for selected camera pos
            ImGui::DragFloat4("Position", glm::value_ptr(m_lightManager.crtLight().m_camera.m_position), 0.1f, -10.0f, 10.0f);

            // Color picker for selected light
            ImGui::ColorEdit4("Color", &m_lightManager.crtLight().m_color[0]);
        }
        ImGui::End();
    }

    // In here you can handle key presses
    // key - Integer that corresponds to numbers in https://www.glfw.org/docs/latest/group__keys.html
    // mods - Any modifier keys pressed, like shift or control
    void onKeyPressed(int key, int mods)
    {
        if (key == GLFW_KEY_M)
        {
            showMinimap = !showMinimap;
        }
        if (key == GLFW_KEY_V)
        {
            if (currentCameraMode == CameraMode::FlyCamera)
            {
            currentCameraMode = CameraMode::ThirdPersonCamera;
            }
            else if (currentCameraMode == CameraMode::ThirdPersonCamera)
            {
            currentCameraMode = CameraMode::FlyCamera;
            }
        }

        if (key == GLFW_KEY_1) {
            currentCameraMode = CameraMode::FlyCamera;
        } else if (key == GLFW_KEY_2) {
            currentCameraMode = CameraMode::ThirdPersonCamera;
        } else if (key == GLFW_KEY_3) {
            currentCameraMode = CameraMode::MinimapCamera;
        } else if (key == GLFW_KEY_4) {
            currentCameraMode = CameraMode::LightCamera;
        }
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
    Shader m_lightShader;
    Shader m_skyboxShader;
    Shader m_cubeShader;

    std::vector<GPUMesh> m_meshes;
    Texture m_texture;
    Texture m_normalMap;
    bool m_useMaterial{true};

    lum::LightManager m_lightManager;

    BezierCurve m_curve;

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
