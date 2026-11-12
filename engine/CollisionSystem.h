#pragma once

#include <algorithm>
#include <string>

#include "CollisionComponent.h"
#include "LocationComponent.h"
#include "SpriteComponent.h"
#include "SystemManager.h"

// =======TODO=========
// Dynamic vs. dynamic: star pickup and star-star collisions are skipped
// Stars hitting statics: ResolveCollision zeroes the velocity, but the spec wants stars to bounce and lose speed. A flag on CollisionComponent, or checking for a SpawnComponent, could switch between stop and bounce.
// doesDamage: spikes aren’t handled.
// Screen shake and landing sound: trigger both in ResolveCollision using the speed before it’s zeroed.


namespace ECSEngine {

template <typename... Components>
class CollisionSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CollisionSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override { 
        auto& entityManager = engine->GetEntityManager();
        // 1: Update current collision boxes
        EntityID entity = 0;
        // MovementComponent has no entity field so we loop through each entity and check if it has a MovementComponent
        for (auto it = entityManager.begin(); it != entityManager.end(); ++it, ++entity) {
            if (!entityManager.IsValid(entity)) {
                continue;
            }
            // if the entity does not have a CollisionComponent, LocationComponent, or SpriteComponent, skip it
            if (!entityManager.template HasComponent<CollisionComponent>(entity)) {
                continue;
            }
            auto& collision = entityManager.template GetComponent<CollisionComponent>(entity);
            // Synchronize the box with the entity's location
            if (entityManager.template HasComponent<LocationComponent>(entity) && entityManager.template HasComponent<SpriteComponent>(entity)) {
                auto& location = entityManager.template GetComponent<LocationComponent>(entity);

                auto& sprite = entityManager.template GetComponent<SpriteComponent>(entity);

                if (sprite.worldSpace) {
                    UpdateCollisionBox(collision, location, sprite);
                }
            }
        }
        // 2: Reset collision flags
        entity = 0;
        for (auto it = entityManager.begin(); it != entityManager.end(); ++it, ++entity) {
            if (!entityManager.IsValid(entity)) {
                continue;
            }
            if (!entityManager.template HasComponent<CollisionComponent>(entity)) {
                continue;
            }
            auto& collision = entityManager.template GetComponent<CollisionComponent>(entity);

            collision.onGround = false;
            collision.onCeiling = false;
            collision.onLeftWall = false;
            collision.onRightWall = false;
        }
         // 3: Check pairs of entities
        EntityID entityA = 0;
        // Loop through each entity and check for collisions with other entities
        for (auto itA = entityManager.begin(); itA != entityManager.end(); ++itA, ++entityA) {
            if (!entityManager.IsValid(entityA)) {
                continue;
            }
            if (!entityManager.template HasComponent<CollisionComponent>(entityA)) {
                continue;
            }
            auto& collisionA = entityManager.template GetComponent<CollisionComponent>(entityA);

            // Static entities do not need to be moved
            if (collisionA.staticEntity) {
                continue;
            }
            // We need location and movement to resolve the collision
            if (!entityManager.template HasComponent<LocationComponent>(entityA) || !entityManager.template HasComponent<MovementComponent>(entityA)) {
                continue;
            }

            auto& locationA = entityManager.template GetComponent<LocationComponent>(entityA);
            auto& movementA = entityManager.template GetComponent<MovementComponent>(entityA);

            EntityID entityB = 0;
            for (auto itB = entityManager.begin(); itB != entityManager.end(); ++itB, ++entityB) {
                if (entityB == entityA || !entityManager.IsValid(entityB)) {
                    continue;
                }
                if (!entityManager.template HasComponent<CollisionComponent>(entityB)) {
                    continue;
                }
                auto& collisionB = entityManager.template GetComponent<CollisionComponent>(entityB);
                // Ignore static-static and dynamic-dynamic pairs for now
                if (!collisionB.staticEntity) {
                    continue;
                }
                // Check whether the two rectangles overlap
                if (!RectRect(collisionA.collisionBox, collisionB.collisionBox)) {
                    continue;
                }
                // Resolve entityA against the static obstacle entityB.
                ResolveCollision(locationA, movementA, collisionA, collisionB);
            }
        }
        return kContinue; 
    }
};

}  // namespace ECSEngine
