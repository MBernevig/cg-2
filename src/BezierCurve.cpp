//
// Created by codri on 07/11/2024.
//

#include "BezierCurve.h"
#include "BezierCurve.h"
#include "BezierCurve.h"
#include "BezierCurve.h"
#include "BezierCurve.h"
#include "BezierCurve.h"
#include "BezierCurve.h"

#include <utility>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

BezierCurve::BezierCurve(std::vector<glm::vec3> ControlPoints):
    m_controlPoints(std::move(ControlPoints)) {
    glGenBuffers(1, &m_VBO);
    glGenVertexArrays(1, &m_VAO);

    // update at start, and every time we change control points
    updateCurvePoints();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), static_cast<void *>(0));
    glEnableVertexAttribArray(0);
}

glm::vec3 BezierCurve::calculateBezierPoint(float t) {
    // (1 - t)^3 P0 + 3(1 - t)^2 * t P1 + 3(1 - t)t^2 P2 + t^3 P3
    float t2 = t * t;
    float t3 = t2 * t;
    float d1 = 1.0f - t;
    float d2 = d1 * d1;
    float d3 = d2 * d1;

    glm::vec3 term0 = d3 * m_controlPoints[0];
    glm::vec3 term1 = 3 * d2 * t * m_controlPoints[1];
    glm::vec3 term2 = 3 * d1 * t2 * m_controlPoints[2];
    glm::vec3 term3 = t3 * m_controlPoints[3];

    return term0 + term1 + term2 + term3;
}

void BezierCurve::drawCurve(const Shader& bezierShader, glm::mat4 mvp) {
    bezierShader.bind();

    glEnable(GL_PROGRAM_POINT_SIZE);
    glUniformMatrix4fv(bezierShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(m_VAO);
    glDrawArrays(GL_LINE_STRIP, 0, m_curvePoints.size());
}

void BezierCurve::updateCurvePoints() {
    m_curvePoints.clear();
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    for (float t = 0; t <= 1.0f; t += 0.02f) {
        m_curvePoints.emplace_back(calculateBezierPoint(t));
    }

    glBufferData(GL_ARRAY_BUFFER, m_curvePoints.size() * sizeof(glm::vec3), m_curvePoints.data(), GL_STATIC_DRAW);
}
