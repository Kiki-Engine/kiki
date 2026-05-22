#pragma once
#include <entt/entt.hpp>

struct TriggerEvent {
    entt::entity trigger = entt::null;
    entt::entity actor   = entt::null;
    int          crossingDir = 0;
};
