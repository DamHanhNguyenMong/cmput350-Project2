#pragma once

#include <algorithm>
#include <string>

#include "Keyboard.h"
#include "LocationComponent.h"
#include "GravityComponent.h"
#include "CollisionComponent.h"
#include "SystemManager.h"
#include "MovementComponent.h"

namespace ECSEngine {

const float kMaxRunVelocity = 15;
const float kDashVelocity = 50;
const float kJumpVelocity = 22;
const float kGroundRunAcceleration = 5;
const float kAirRunAcceleration = 4;
const int kDashInterval = 60;
const int kDashDuration = 8;
const float kWallJumpKick = 14;
const int kWallJumpLock = 10; 
const float kWallClimbKick = 5; // small hop off the same wall
const int kWallClimbLock = 3; // short lock, then holding toward the wall pulls you back
const float kWallClimbJump = 18; // climb hop off the same wall
const float kMaxAirVelocity = 10; 
const int kWallGrace = 10; 

template <typename... Components>
class InputSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("InputSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override { 
        auto& entityManager = engine->GetEntityManager();

        auto& inputStorage = entityManager.template GetComponentStorage<InputComponent>();

        // Process input components
        for (size_t i = 0; i < inputStorage.mStorage.size(); ++i) {
            // Skip removed input components
            if (!inputStorage.mValid[i]) {
                continue;
            }
            InputComponent& input = inputStorage.mStorage[i]; // Get the input component from storage
            EntityID entity = input.entity; // Get the associated entity ID

            // Make sure the entity has the components we need: the entity must have both a MovementComponent and a CollisionComponent 
            // to process input for movement and collision detection
            if (!entityManager.template HasComponent<MovementComponent>(entity) || !entityManager.template HasComponent<CollisionComponent>(entity)) {
                continue;
            }
            MovementComponent& movement = entityManager.template GetComponent<MovementComponent>(entity);
            CollisionComponent& collision = entityManager.template GetComponent<CollisionComponent>(entity);

            // Check whether the left and right arrow keys are pressed
            bool moveLeft = input.keydown.test(Key::Left);
            bool moveRight = input.keydown.test(Key::Right);
            // And the jump and dash keys
            bool jumpHeld = input.keydown.test(Key::Z);
            bool dashHeld = input.keydown.test(Key::C);
            // Detect newly pressed keys
            bool jumpPressed = jumpHeld && !input.lastKeydown.test(Key::Z);
            bool dashPressed = dashHeld && !input.lastKeydown.test(Key::C);

            // 1. Horizontal movement (not while dashing)
            if (input.dashTimer == 0 && input.wallJumpTimer == 0) { // not dashing and not in wall jump lock
                // if on ground, use ground acceleration; if in air, use air acceleration
                float acceleration = collision.onGround ? kGroundRunAcceleration : kAirRunAcceleration;
                if (moveLeft) {
                    movement.velocity.x -= acceleration;
                }
                if (moveRight) {
                    movement.velocity.x += acceleration;
                }
                // If not moving left or right, apply friction/drag to slow down the player
                if (!moveLeft && !moveRight) {
                    movement.velocity.x *= collision.onGround ? 0.3f : 0.85f;   // ground friction / air drag
                    if (std::abs(movement.velocity.x) < 0.5f) movement.velocity.x = 0;
                }
                float maxSpeed = collision.onGround ? kMaxRunVelocity : kMaxAirVelocity;
                movement.velocity.x = std::clamp(movement.velocity.x, -maxSpeed, maxSpeed);
            }

            // Remember wall contact for a few frames so the wall jump still works after pressing away
            if (collision.onGround) {
                input.wallGraceTimer = 0;
            }
            else if (collision.onLeftWall) {
                input.wallSide = -1;
                input.wallGraceTimer = kWallGrace;
            }
            else if (collision.onRightWall) {
                input.wallSide = 1;
                input.wallGraceTimer = kWallGrace;
            }
            else if (input.wallGraceTimer > 0) {
                --input.wallGraceTimer;
            }

            // 2. Jumping and wall jumping
            if (jumpPressed) {
                // Normal ground jump
                if (collision.onGround) {
                    movement.velocity.y = -kJumpVelocity;
                    engine->GetSoundManager().GetSound("jump").play();
                }
                // Wall jump: allowed any time you touch a wall in the air, falling or rising
                else if (input.wallGraceTimer > 0 && input.wallSide == -1) {
                    bool climbing = moveLeft;
                    movement.velocity.x = climbing ? kWallClimbKick : kWallJumpKick;   // push right
                    movement.velocity.y = -(climbing ? kWallClimbJump : kJumpVelocity);
                    input.wallJumpTimer = climbing ? kWallClimbLock : kWallJumpLock;
                    input.wallGraceTimer = 0; // use it up so it can't fire twice
                    engine->GetSoundManager().GetSound("jump").play();
                }
                else if (input.wallGraceTimer > 0 && input.wallSide == 1) {
                    bool climbing = moveRight;
                    movement.velocity.x = -(climbing ? kWallClimbKick : kWallJumpKick); // push left
                    movement.velocity.y = -(climbing ? kWallClimbJump : kJumpVelocity);
                    input.wallJumpTimer = climbing ? kWallClimbLock : kWallJumpLock;
                    input.wallGraceTimer = 0;
                    engine->GetSoundManager().GetSound("jump").play();
                }
            }
            // 3. Variable jump height
            // Stop upward movement when Z is released
            if (!jumpHeld && movement.velocity.y < 0) {
                movement.velocity.y *= 0.5f; 
            }
            // 4. Dash cooldown
            if (input.lastDash > 0) {
                --input.lastDash;
            }
            // Dash duration
            if (input.dashTimer > 0) {
                --input.dashTimer;
            }
            // decrement wall jump timer
            if (input.wallJumpTimer > 0) {
                --input.wallJumpTimer;
            }

            // 5. Start a dash
            if (dashPressed && input.lastDash == 0) { // new dash (no last dash)
                float dashDirection = 0;
                if (moveLeft && !moveRight) {
                    dashDirection = -1;
                }
                else if (moveRight && !moveLeft) {
                    dashDirection = 1;
                }
                else if (movement.velocity.x < 0) {
                    // If no direction keys are pressed, use the current velocity to determine dash direction
                    dashDirection = -1;
                }
                else {
                    // Default direction: right
                    dashDirection = 1;
                }

                movement.velocity.x = dashDirection * kDashVelocity; // Set horizontal velocity to dash speed in the chosen direction

                // Stop vertical movement during horizontal dash
                movement.velocity.y = 0;

                input.dashTimer = kDashDuration;
                input.lastDash = kDashInterval;
                engine->GetSoundManager().GetSound("dash").play();
                // Make sure the player wont immediately start falling while dashing
                if (entityManager.template HasComponent<GravityComponent>(entity)) {
                    auto& gravity = entityManager.template GetComponent<GravityComponent>(entity);
                    gravity.disabledTimer = kDashDuration; // disable gravity for the duration of the dash
                }
            }
            // 6. Apply velocity limits
            if (input.dashTimer > 0) { // During dash, allow higher velocity
                // Preserve dash velocity
                movement.velocity.x = std::clamp(movement.velocity.x, -kDashVelocity, kDashVelocity);
            }
            else {
                // Normal running velocity
                movement.velocity.x =  std::clamp(movement.velocity.x, -kMaxRunVelocity, kMaxRunVelocity);
            }

        }
        return kContinue; 
    }
};

}  // namespace ECSEngine
