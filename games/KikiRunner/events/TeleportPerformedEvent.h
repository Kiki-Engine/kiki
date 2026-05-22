#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct TeleportPerformedEvent {
    entt::entity actor = entt::null;
    entt::entity src   = entt::null;
    entt::entity dst   = entt::null;

    glm::vec3 oldPosition = { 0.f, 0.f, 0.f };
    glm::vec3 newPosition = { 0.f, 0.f, 0.f };

    glm::quat srcRotation = glm::quat(1.f, 0.f, 0.f, 0.f);
    glm::quat dstRotation = glm::quat(1.f, 0.f, 0.f, 0.f);

    glm::quat deltaRotation = glm::quat(1.f, 0.f, 0.f, 0.f);

    float yawDeltaDegrees = 0.0f;
};
