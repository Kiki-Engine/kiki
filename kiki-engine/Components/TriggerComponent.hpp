#pragma once
#include <glm/glm.hpp>

struct TriggerComponent {
    glm::vec3 halfExtents = { 1.0f, 1.0f, 0.1f };
    bool      active = true;
};

struct GoalTag {};

struct TeleportTag {
    int loopNum = 0;
    int order   = 0;
};
