#pragma once

#include <framework/disable_all_warnings.h>
#include <framework/mesh.h>
#include <framework/shader.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()

#include <exception>
#include <filesystem>
#include <framework/opengl_includes.h>

struct MeshLoadingException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Alignment directives are to comply with std140 alignment requirements (https://www.khronos.org/opengl/wiki/Interface_Block_(GLSL)#Memory_layout)
struct GPUMaterial {
    GPUMaterial(const Material& material);

    alignas(16) glm::vec3 kd{ 1.0f };
	alignas(16) glm::vec3 ks{ 0.0f };
	float shininess{ 1.0f };
	float transparency{ 1.0f };
};

class GPUMesh {
public:
    GPUMesh(const Mesh& cpuMesh);
    // Cannot copy a GPU mesh because it would require reference counting of GPU resources.
    GPUMesh(const GPUMesh&) = delete;
    GPUMesh(GPUMesh&&);
    ~GPUMesh();

    // Generate a number of GPU meshes from a particular model file.
    // Multiple meshes may be generated if there are multiple sub-meshes in the file
    static std::vector<GPUMesh> loadMeshGPU(std::filesystem::path filePath, bool normalize = false);

    // Cannot copy a GPU mesh because it would require reference counting of GPU resources.
    GPUMesh& operator=(const GPUMesh&) = delete;
    GPUMesh& operator=(GPUMesh&&);

    bool hasTextureCoords() const;

    // Bind VAO and call glDrawElements.
    void draw(const Shader& drawingShader);

    void translate(const glm::vec3& offset);
    void rotate(float angle, const glm::vec3& axis);
    void scale(const glm::vec3& scaleFactors);
    void attachToCamera(
        const glm::vec3 &cameraPos,
        const glm::vec3 &cameraForward,
        const glm::vec3 &cameraUp,
        const glm::vec3 &offset = glm::vec3(0.0f));

        glm::mat4 modelMatrix { 1.0f };

private:
    void moveInto(GPUMesh&&);
    void freeGpuMemory();

private:
    static constexpr GLuint INVALID = 0xFFFFFFFF;

    GLsizei m_numIndices { 0 };
    bool m_hasTextureCoords { false };
    GLuint m_ibo { INVALID };
    GLuint m_vbo { INVALID };
    GLuint m_vao { INVALID };
    GLuint m_uboMaterial { INVALID };
    
};
