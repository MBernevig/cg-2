#pragma once
#include <vector>
#include <framework/shader.h>
#include <glm/vec3.hpp>

class BezierCurve {
public:
    BezierCurve(std::vector<glm::vec3> ControlPoints);

    void drawCurve(const Shader &bezierShader, glm::mat4 mvp);
    void updateCurvePoints();

    glm::vec3 calculateBezierPoint(float t);
    
public:
    std::vector<glm::vec3> m_controlPoints;
    std::vector<glm::vec3> m_curvePoints;
    GLuint m_VBO;
    GLuint m_VAO;
};

