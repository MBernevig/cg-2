#pragma once

#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()


struct Config {
    // Particle simulation parameters
    uint32_t numParticles       = 25;
    float particleSimTimestep   = 0.014f;
    float particleRadius        = 0.45f;
    bool particleInterCollision = true;

    // Particle simulation flags
    bool doSingleStep           = false;
    bool doContinuousSimulation = true;
    bool doResetSimulation      = false;

    // Container sphere parameters
    glm::vec3 sphereCenter          = glm::vec3(0.0f);
    float sphereRadius              = 3.0f;
    glm::vec3 sphereColor           = glm::vec3(1.0f);

	// ======Extras======

    bool addDamping = false;
	float dampingFactor = 0.1f;
	bool collisionsCauseRelativeVelocity = false;



    // ===== Part 2: Drawing =====

	glm::vec3 lowSpeedColor = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 highSpeedColor = glm::vec3(1.0f, 0.0f, 0.0f);
	float maxSpeed = 5.f;
    
	bool useShading = true;

	float ambientCoefficient = 0.1f;
	glm::vec3 lightColor = glm::vec3(1.0f);

	// ===== Part 3: Bounce =====

    float maxCollisions = 5;
    float colorFrames = 10;
	glm::vec3 blinkColor = glm::vec3(0.0f, 1.0f, 0.0f);
    bool enableBlink = false;

};
