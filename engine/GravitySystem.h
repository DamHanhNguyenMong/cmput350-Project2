#pragma once

#include <algorithm>
#include <string>

#include "InputComponent.h"
#include "CollisionComponent.h"
#include "Keyboard.h"
#include "LocationComponent.h"
#include "SystemManager.h"

namespace ECSEngine
{
// Assumed constants
const float kMaxFallVelocity = 35; // Maximum falling speed
const float kFastFallVelocity = 50; // Maximum falling speed when fast falling
const float kWallSlideVelocity = 4; // Maximum falling speed when sliding down a wall

template <typename... Components>
class GravitySystem : public System<Components...> {
public:
	const std::string & GetName() const override
	{ static std::string name("GravitySystem"); return name; }
	
	tSystemReturn Run(EngineView<Components...> *engine) override
	{
		auto& entityManager = engine->GetEntityManager();

		EntityID entity = 0;
		// GravityComponent has no entity field so we loop through each entity and check if it has a GravityComponent
		for (auto it = entityManager.begin(); it != entityManager.end(); ++it, ++entity) {
            if (!entityManager.IsValid(entity)) {
                continue;
            }
			// Entity must have both Gravity and Movement components
            if (!entityManager.template HasComponent<GravityComponent>(entity) || !entityManager.template HasComponent<MovementComponent>(entity)) {
                continue;
            }
			GravityComponent& gravity = entityManager.template GetComponent<GravityComponent>(entity);
            MovementComponent& movement = entityManager.template GetComponent<MovementComponent>(entity);
			// Get collision information if available
            bool onGround = false;
            bool onCeiling = false;
            bool onLeftWall = false;
            bool onRightWall = false;
			if (entityManager.template HasComponent<CollisionComponent>(entity)) {
				// Get the collision component to check if the entity is on the ground, ceiling, or walls
                CollisionComponent& collision = entityManager.template GetComponent<CollisionComponent>(entity);
                onGround = collision.onGround;
                onCeiling = collision.onCeiling;
                onLeftWall = collision.onLeftWall;
                onRightWall = collision.onRightWall;
            }
			// 1. Handle disabled gravity
			if (gravity.disabledTimer > 0) {
                --gravity.disabledTimer; // Decrement the disabled timer
                continue;
            }
			// 2. Handle grounded and ceiling states
			// Prevent falling through the ground
            if (onGround && movement.velocity.y > 0) { // If the entity is on the ground and moving downward
                movement.velocity.y = 0;
            }
			// Prevent moving upward through the ceiling
            if (onCeiling && movement.velocity.y < 0) { // If the entity is on the ceiling and moving upward
                movement.velocity.y = 0;
            }
			// 3. Apply gravity
			float accelerationY = gravity.acceleration.y;
            if (movement.velocity.y > 0) {
                accelerationY *= 1.5f;   // fall gravity multiplier
                // Holding Down while falling speeds up the fall even more
                if constexpr (IsOneOf<InputComponent, Components...>) {
                    if (entityManager.template HasComponent<InputComponent>(entity) &&
                        entityManager.template GetComponent<InputComponent>(entity).keydown.test(Key::Down)) {
                        accelerationY *= 1.5f;
                    }
                }
            }
			movement.velocity.y += accelerationY;

			// 4. Limit falling speed
			// When moving downward
			if (movement.velocity.y > 0) {
				bool pushingIntoWall = false;
                bool fastFall = false;
                // Only entities with input (the player) can wall slide or fast fall
                if constexpr (IsOneOf<InputComponent, Components...>) {
                    if (entityManager.template HasComponent<InputComponent>(entity)) {
                        InputComponent& input = entityManager.template GetComponent<InputComponent>(entity);
                        // Wall slide only when the arrow key points into the wall being touched
                        pushingIntoWall = (onLeftWall && input.keydown.test(Key::Left)) ||
                                          (onRightWall && input.keydown.test(Key::Right));
                        fastFall = input.keydown.test(Key::Down);
                    }
                }
                float maxFall = kMaxFallVelocity;
                if (pushingIntoWall) {
                    maxFall = kWallSlideVelocity; // slow slide down the wall
                }
                else if (fastFall) {
                    maxFall = kFastFallVelocity; // holding Down
                }
                movement.velocity.y = std::min(movement.velocity.y, maxFall);
			}

		}
		return kContinue;
	}
	
};


}



