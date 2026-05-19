#pragma once
#include <glm/glm.hpp>
#include "Animation/AnimationComponent.h"

enum class Ability : uint32_t {
    None = 0,
    DoubleJump = 1 << 0,  // 0b0001
    SpeedBoost = 1 << 1,  // 0b0010
    Dash = 1 << 2,  // 0b0100
    //WallJump = 1 << 3,  // 0b1000
    //Glide = 1 << 4,  // 0b10000
};
inline Ability operator|(Ability a, Ability b) {
    return static_cast<Ability>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline Ability operator&(Ability a, Ability b) {
    return static_cast<Ability>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
inline Ability operator~(Ability a) {
    return static_cast<Ability>(~static_cast<uint32_t>(a));
}
inline Ability& operator|=(Ability& a, Ability b) {
    return a = a | b;
}
inline Ability& operator&=(Ability& a, Ability b) {
    return a = a & b;
}
struct CharacterComponent {
    float walkSpeed = 4.0f;
    float runSpeed = 8.0f;
    float jumpForce = 8.0f;
	float rotateSpeed = 10.0f; // character rotation speed (for interpolating facing direction)

    float acceleration = 18.0f;   // ground accelerate rate towards target velocity
    float deceleration = 22.0f;   // ground stopping rate when no input
    float airControl   = 5.0f;    // air horizontal control rate

    float currentMaxSpeed = walkSpeed;

    float jumpTimer = 0.0f;

    Kiki::CharacterState state = Kiki::CharacterState::Idle;

    //CharacterState state = CharacterState::Idle;
    Ability abilities = Ability::None;
    glm::vec3      velocity = { 0, 0, 0 };
	glm::vec3      spawnPosition = { 0, 0, 0 }; // for respawning after falling off the level
	float          facingYaw = 0.0f;   // current facing direction
	float          targetYaw = 0.0f;   // target facing direction

    std::vector<float> timeLimits = { 120.f,100.f,70.f,40.f }; // time limits for each ability, in the same order as the Ability enum
    std::vector<std::u32string> labels = {
            U"double jump",
            U"speed boost",
            U"dash",
            U"completed"
    };
    std::vector<bool> isDone = {false,false,false,false};

    bool hasAbility(Ability ability) const {
        return (abilities & ability) == ability;
    }

    void grantAbility(Ability ability) {
        abilities |= ability;
    }

    void revokeAbility(Ability ability) {
        abilities &= ~ability;
    }
};