#pragma once

// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <framework/window.h>

class Camera {
public:
    // Constructor: Initializes the camera with a window pointer.
    // @param pWindow: Pointer to the window.
    Camera(Window* pWindow);

    // Constructor: Initializes the camera with a window pointer, position, and forward direction.
    // @param pWindow: Pointer to the window.
    // @param position: Initial position of the camera.
    // @param forward: Initial forward direction of the camera.
    Camera(Window* pWindow, const glm::vec3& position, const glm::vec3& forward);

    // Updates the camera based on user input.
    void updateInput();

    // Enables or disables user interaction with the camera.
    // @param enabled: True to enable interaction, false to disable.
    void setUserInteraction(bool enabled);

    // Returns the current position of the camera.
    // @return: The position of the camera.
    glm::vec3 cameraPos() const;

    // Returns the view matrix of the camera.
    // @return: The view matrix.
    glm::mat4 viewMatrix() const;

    // Decays the camera's velocity by a given factor.
    // @param decayFactor: The factor by which to decay the velocity. Setting to 0 instantly stops velocity. Setting to 0.9 means 10% reduction per frame.
    void decayVelocity(float decayFactor);

private:
    // Rotates the camera around the X-axis.
    // @param angle: The angle to rotate by.
    void rotateX(float angle);

    // Rotates the camera around the Y-axis.
    // @param angle: The angle to rotate by.
    void rotateY(float angle);

public:
    static constexpr glm::vec3 s_yAxis { 0, 1, 0 }; // The world up direction.
    glm::vec3 m_position { 0 }; // The position of the camera.
    glm::vec3 m_forward { 0, 0, -1 }; // The forward direction of the camera.
    glm::vec3 m_up { 0, 1, 0 }; // The up direction of the camera.
    glm::vec3 m_velocity{0, 0, 0}; // The velocity of the camera.

    const Window* m_pWindow; // Pointer to the window.
    bool m_userInteraction { true }; // Whether user interaction is enabled.
    glm::dvec2 m_prevCursorPos { 0 }; // The previous cursor position.
};;
/**
 * @brief Sets the position of the camera.
 * @param position The new position of the camera.
 */
void setPosition(const glm::vec3& position);

/**
 * @brief Sets the forward direction of the camera.
 * @param forward The new forward direction of the camera.
 */
void setForward(const glm::vec3& forward);