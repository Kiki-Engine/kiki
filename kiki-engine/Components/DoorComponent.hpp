#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct DoorComponent {
    enum class State { Closed, Opening, Open, Closing };

    State state = State::Closed;

    float triggerRadius   = 2.0f;
    float openAngleDeg    = 90.0f;
    float speedDegPerSec  = 540.0f;

    float currentAngleDeg = 0.0f;

    // Rotation axis in local space (door hinge axis). Default: world up (Y).
    glm::vec3 axis = { 0.0f, 1.0f, 0.0f };

    // Captured at scene load: the rotation when the door is fully closed.
    glm::quat closedRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    // +1 or -1, decided at the moment the door starts to open so it swings
    // toward the side the player came from.
    float openSign = 1.0f;
};
