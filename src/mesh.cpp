#include "mesh.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <fmt/format.h>
DISABLE_WARNINGS_POP()
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // for glm::translate, glm::rotate, etc.
#include <glm/gtc/type_ptr.hpp>

GPUMaterial::GPUMaterial(const Material& material) :
    kd(material.kd),
    ks(material.ks),
    shininess(material.shininess),
    transparency(material.transparency) {
}

GPUMesh::GPUMesh(const Mesh& cpuMesh)
{

    m_modelMatrix = glm::mat4(1.0f); // Identity matrix by default

    // Create uniform buffer to store mesh material (https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL)
    GPUMaterial gpuMaterial(cpuMesh.material);
    glGenBuffers(1, &m_uboMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, m_uboMaterial);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GPUMaterial), &gpuMaterial, GL_STATIC_READ);

    // Figure out if this mesh has texture coordinates
    m_hasTextureCoords = static_cast<bool>(cpuMesh.material.kdTexture);

    if (cpuMesh.material.kdTexture != nullptr) {
        texture = std::make_shared<Texture>(cpuMesh.material.kdTexture);
    }
    
    if (cpuMesh.material.normalTexture != nullptr) {
        normalMap = std::make_shared<Texture>(cpuMesh.material.normalTexture);
    }

    // Create VAO and bind it so subsequent creations of VBO and IBO are bound to this VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // Create vertex buffer object (VBO)
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(cpuMesh.vertices.size() * sizeof(decltype(cpuMesh.vertices)::value_type)), cpuMesh.vertices.data(), GL_STATIC_DRAW);

    // Create index buffer object (IBO)
    glGenBuffers(1, &m_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(cpuMesh.triangles.size() * sizeof(decltype(cpuMesh.triangles)::value_type)), cpuMesh.triangles.data(), GL_STATIC_DRAW);

    // Tell OpenGL that we will be using vertex attributes 0, 1 and 2.
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    // We tell OpenGL what each vertex looks like and how they are mapped to the shader (location = ...).
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, tangent));
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, bitangent));
    // Reuse all attributes for each instance
    glVertexAttribDivisor(0, 0);
    glVertexAttribDivisor(1, 0);
    glVertexAttribDivisor(2, 0);

    // Each triangle has 3 vertices.
    m_numIndices = static_cast<GLsizei>(3 * cpuMesh.triangles.size());
}

GPUMesh::GPUMesh(GPUMesh&& other)
{
    moveInto(std::move(other));
}

GPUMesh::~GPUMesh()
{
    freeGpuMemory();
}

GPUMesh& GPUMesh::operator=(GPUMesh&& other)
{
    moveInto(std::move(other));
    return *this;
}

std::vector<GPUMesh> GPUMesh::loadMeshGPU(std::filesystem::path filePath, bool normalize) {
    if (!std::filesystem::exists(filePath))
        throw MeshLoadingException(fmt::format("File {} does not exist", filePath.string().c_str()));

    // Generate GPU-side meshes for all sub-meshes
    std::vector<Mesh> subMeshes = loadMesh(filePath, normalize);
    std::vector<GPUMesh> gpuMeshes;
    for (const Mesh& mesh : subMeshes) { gpuMeshes.emplace_back(mesh); }
    
    return gpuMeshes;
}

bool GPUMesh::hasTextureCoords() const
{
    return m_hasTextureCoords;
}

void GPUMesh::draw(const Shader& drawingShader)
{
    // Bind material data uniform (we assume that the uniform buffer objects is always called 'Material')
    // Yes, we could define the binding inside the shader itself, but that would break on OpenGL versions below 4.2
    drawingShader.bindUniformBlock("Material", 0, m_uboMaterial);
    glUniformMatrix4fv(drawingShader.getUniformLocation("modelMatrix"),1,GL_FALSE, glm::value_ptr(m_modelMatrix));
    
    // Draw the mesh's triangles
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, nullptr);
}

glm::mat4 GPUMesh::modelMatrix()
{
    glm::mat4 parentMatrixNoScale = glm::mat4(1.0f);
    parentMatrixNoScale[3] = parentMatrix[3]; // Copy translation
    parentMatrixNoScale[0] = glm::normalize(parentMatrix[0]); // Copy and normalize x-axis
    parentMatrixNoScale[1] = glm::normalize(parentMatrix[1]); // Copy and normalize y-axis
    parentMatrixNoScale[2] = glm::normalize(parentMatrix[2]); // Copy and normalize z-axis
    return parentMatrixNoScale * m_modelMatrix;
}


void GPUMesh::setParent(GPUMesh &other)
{
    parentMatrix = other.modelMatrix();
}

void GPUMesh::moveInto(GPUMesh &&other)
{
    freeGpuMemory();
    m_numIndices = other.m_numIndices;
    m_hasTextureCoords = other.m_hasTextureCoords;
    m_ibo = other.m_ibo;
    m_vbo = other.m_vbo;
    m_vao = other.m_vao;
    m_uboMaterial = other.m_uboMaterial;

    texture = other.texture;
    m_modelMatrix = other.m_modelMatrix;
    normalMap = other.normalMap;
    renderFPV  = other.renderFPV;
    parentMatrix = other.parentMatrix;
    drawMe = other.drawMe;

    other.m_numIndices = 0;
    other.m_hasTextureCoords = other.m_hasTextureCoords;
    other.m_ibo = INVALID;
    other.m_vbo = INVALID;
    other.m_vao = INVALID;
    other.m_uboMaterial = INVALID;
}

void GPUMesh::freeGpuMemory()
{
    if (m_vao != INVALID)
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != INVALID)
        glDeleteBuffers(1, &m_vbo);
    if (m_ibo != INVALID)
        glDeleteBuffers(1, &m_ibo);
    if (m_uboMaterial != INVALID)
        glDeleteBuffers(1, &m_uboMaterial);
}

void GPUMesh::translate(const glm::vec3& offset) {
    m_modelMatrix = glm::translate(m_modelMatrix, offset);
}

void GPUMesh::rotate(float angle, const glm::vec3& axis) {
    m_modelMatrix = glm::rotate(m_modelMatrix, angle, axis);
}

void GPUMesh::scale(const glm::vec3& scaleFactors) {
    m_modelMatrix = glm::scale(m_modelMatrix, scaleFactors);
}


void GPUMesh::attachToCamera(
    const glm::vec3& cameraPos, 
    const glm::vec3& cameraForward, 
    const glm::vec3& cameraUp,
    const glm::vec3& offset)  // Offset parameter with default (0,0,0)
{
    // First, translate the model to the camera's position
    m_modelMatrix = glm::translate(glm::mat4(1.f), cameraPos);
    // Calculate the angle of rotation around the y-axis
    float angleY = atan2(cameraForward.x, cameraForward.z); // Use atan2 for better angle calculation
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angleY, glm::vec3(0.0f, 1.0f, 0.0f));
    m_modelMatrix *= rotationMatrix;

    // Apply offset translation to the head position
    m_modelMatrix = glm::translate(m_modelMatrix, offset);



    // Create a rotation matrix only around the Y axis

    // Apply rotation

    // Translate back by the inverse of the offset to maintain the correct positioning
    // m_modelMatrix = glm::translate(m_modelMatrix, -offset);
}