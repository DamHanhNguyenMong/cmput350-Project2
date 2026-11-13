#pragma once

#include <bitset>
#include <unordered_map>

#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window/Keyboard.hpp>

namespace ECSEngine {

struct InputComponent {
    InputComponent(EntityID whichEntity) : entity(whichEntity), lastDash(0), dashTimer(0), wallJumpTimer(0), wallGraceTimer(0), wallSide(0) {}
    EntityID entity;
    int lastDash;  // Remaining dash cooldown (frames)
    int dashTimer; // Remaining dash duration (frames)
    int wallJumpTimer; // Frames left where horizontal input is ignored after a wall jump
    int wallGraceTimer; // Frames left where a wall jump is still allowed after leaving a wall
    int wallSide; // Which wall we were last on: -1 = left, 1 = right
    std::bitset<sf::Keyboard::ScancodeCount> keydown;
    std::bitset<sf::Keyboard::ScancodeCount> lastKeydown;
};

}  // namespace ECSEngine
