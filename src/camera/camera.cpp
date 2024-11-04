#include "camera.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
DISABLE_WARNINGS_POP()

float previousRotation = 0.f;

Camera::Camera(Window* pWindow)
    : Camera(pWindow, glm::vec3(0), glm::vec3(0, 0, -1))
{
}

Camera::Camera(Window* pWindow, const glm::vec3& pos, const glm::vec3& forward)
    : m_position(pos)
    , m_forward(glm::normalize(forward))
    , m_pWindow(pWindow)
{
}

void Camera::setUserInteraction(bool enabled)
{
    m_userInteraction = enabled;
}

glm::vec3 Camera::cameraPos() const
{
    return m_position;
}

glm::mat4 Camera::viewMatrix() const
{
    return glm::lookAt(m_position, m_position + m_forward, m_up);
}

void Camera::rotateX(float angle)
{
    const glm::vec3 horAxis = glm::cross(s_yAxis, m_forward);

    m_forward = glm::normalize(glm::angleAxis(angle, horAxis) * m_forward);
    m_up = glm::normalize(glm::cross(m_forward, horAxis));
}

void Camera::rotateY(float angle)
{
    const glm::vec3 horAxis = glm::cross(s_yAxis, m_forward);

    m_forward = glm::normalize(glm::angleAxis(angle, s_yAxis) * m_forward);
    m_up = glm::normalize(glm::cross(m_forward, horAxis));
}

// void Camera::updateInput()
// {
//     float moveSpeed = 0.1f;
//     constexpr float lookSpeed = 0.0050f;

//     if (m_userInteraction) {
//         glm::vec3 localMoveDelta { 0 };
//         const glm::vec3 right = glm::normalize(glm::cross(m_forward, m_up));
//         if(m_pWindow->isKeyPressed(GLFW_KEY_LEFT_SHIFT))
//             moveSpeed = 1.f;
//         if (m_pWindow->isKeyPressed(GLFW_KEY_A))
//             m_position -= moveSpeed * right;
//         if (m_pWindow->isKeyPressed(GLFW_KEY_D))
//             m_position += moveSpeed * right;
//         if (m_pWindow->isKeyPressed(GLFW_KEY_W))
//             m_position += moveSpeed * m_forward;
//         if (m_pWindow->isKeyPressed(GLFW_KEY_S))
//             m_position -= moveSpeed * m_forward;
//         if (m_pWindow->isKeyPressed(GLFW_KEY_SPACE))
//             m_position += moveSpeed * m_up;
//         if (m_pWindow->isKeyPressed(GLFW_KEY_C))
//             m_position -= moveSpeed * m_up;

//         if (m_pWindow->isKeyPressed(GLFW_KEY_E))
//             rotateY(45.f);

//         const glm::dvec2 cursorPos = m_pWindow->getCursorPos();
//         const glm::vec2 delta = lookSpeed * glm::vec2(m_prevCursorPos - cursorPos);
//         m_prevCursorPos = cursorPos;

//         if (m_pWindow->isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
//             if (delta.x != 0.0f)
//                 rotateY(delta.x);
//             if (delta.y != 0.0f)
//                 rotateX(delta.y);
//         }
//     } else {
//         m_prevCursorPos = m_pWindow->getCursorPos();
//     }
// }

void Camera::updateInput()
{
    // Parameters for movement
    float maxSpeed = 1.0f;     // Maximum speed
    float acceleration = 0.05f; // Acceleration rate per frame
    constexpr float decayFactor = 0.60f;  // Decay factor (0.90 means 10% reduction per frame)
    constexpr float lookSpeed = 0.0050f;

    glm::vec3 accelerationVector { 0.0f };

    if (m_userInteraction) {
        const glm::vec3 right = glm::normalize(glm::cross(m_forward, m_up));

        // Apply acceleration based on user input
        if (m_pWindow->isKeyPressed(GLFW_KEY_LEFT_SHIFT))
        {
            acceleration = 1.f;
            maxSpeed = 2.f;
        }
        if (m_pWindow->isKeyPressed(GLFW_KEY_LEFT_ALT))
        {
            maxSpeed = 0.3f;
        }
        if (m_pWindow->isKeyPressed(GLFW_KEY_A))
            accelerationVector -= right * acceleration;
        if (m_pWindow->isKeyPressed(GLFW_KEY_D))
            accelerationVector += right * acceleration;
        if (m_pWindow->isKeyPressed(GLFW_KEY_W))
            accelerationVector += m_forward * acceleration;
        if (m_pWindow->isKeyPressed(GLFW_KEY_S))
            accelerationVector -= m_forward * acceleration;
        if (m_pWindow->isKeyPressed(GLFW_KEY_SPACE))
            accelerationVector += m_up * acceleration;
        if (m_pWindow->isKeyPressed(GLFW_KEY_C))
            accelerationVector -= m_up * acceleration;


        // Update velocity with acceleration, then clamp to max speed
        m_velocity += accelerationVector;
        if (glm::length(m_velocity) > maxSpeed) {
            m_velocity = glm::normalize(m_velocity) * maxSpeed;
        }

        // Apply position update based on current velocity
        m_position += m_velocity;

        // Apply decay when no input is active
        if (accelerationVector == glm::vec3(0.0f)) {
            m_velocity *= decayFactor;
            // Optional: Stop the camera if velocity is almost zero to prevent drift
            if (glm::length(m_velocity) < 0.001f)
                m_velocity = glm::vec3(0.0f);
        }

        // Handle rotation
        const glm::dvec2 cursorPos = m_pWindow->getCursorPos();
        const glm::vec2 delta = lookSpeed * glm::vec2(m_prevCursorPos - cursorPos);
        m_prevCursorPos = cursorPos;

        if (m_pWindow->isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            if (delta.x != 0.0f)
                rotateY(delta.x);
            if (delta.y != 0.0f)
                rotateX(delta.y);
        }
    } else {
        m_prevCursorPos = m_pWindow->getCursorPos();
    }
}


void Camera::decayVelocity(float decayFactor){
     m_velocity *= decayFactor;
}