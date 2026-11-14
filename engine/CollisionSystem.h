#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "CameraShakeComponent.h"
#include "InputComponent.h"
#include "ScoreComponent.h"
#include "CollisionComponent.h"
#include "LocationComponent.h"
#include "SpriteComponent.h"
#include "SystemManager.h"
#include "CameraComponent.h"
#include "Keyboard.h"
#include "MovementComponent.h"
#include "RespawnComponent.h"


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
        // 2: Reset collision flags
        bool playerWasOnGround = false;
        bool playerWasOnWall = false;
        EntityID entity = 0;
        for (auto it = entityManager.begin(); it != entityManager.end(); ++it, ++entity) {
            if (!entityManager.IsValid(entity)) {
                continue;
            }
            if (!entityManager.template HasComponent<CollisionComponent>(entity)) {
                continue;
            }
            auto& collision = entityManager.template GetComponent<CollisionComponent>(entity);
            // Remember the player's flags from last frame before clearing them
            if (entityManager.template HasComponent<InputComponent>(entity)) {
                playerWasOnGround = collision.onGround;
                playerWasOnWall = collision.onLeftWall || collision.onRightWall;
            }
            collision.onGround = false;
            collision.onCeiling = false;
            collision.onLeftWall = false;
            collision.onRightWall = false;
        }
         // 3: Check pairs of entities
        std::vector<EntityID> collected;   // stars to remove after the loops
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
            // The player has an InputComponent; any other dynamic entity is a star
            bool aIsPlayer = entityManager.template HasComponent<InputComponent>(entityA);
            Point2D velBefore = movementA.velocity;   // speed going into the hit, used for shake size


            EntityID entityB = 0;
            for (auto itB = entityManager.begin(); itB != entityManager.end(); ++itB, ++entityB) {
                if (entityB == entityA || !entityManager.IsValid(entityB)) {
                    continue;
                }
                if (!entityManager.template HasComponent<CollisionComponent>(entityB)) {
                    continue;
                }
                auto& collisionB = entityManager.template GetComponent<CollisionComponent>(entityB);
                if (!entityManager.template HasComponent<LocationComponent>(entityB)) continue;
                // locationB is needed to check for collision, and movementB is needed to resolve star-star collisions
                auto& locationB = entityManager.template GetComponent<LocationComponent>(entityB);
                // Check if the two entities are colliding
                if (!RectRect(WorldBox(collisionA.collisionBox, locationA.loc), WorldBox(collisionB.collisionBox, locationB.loc))) {
                    continue;
                } 

                // Dynamic vs static: the player stops, stars bounce
                if (collisionB.staticEntity) {
                    // Spikes hurt the player: teleport back to the respawn point
                    if (collisionB.doesDamage && aIsPlayer && entityManager.template HasComponent<RespawnComponent>(entityA)) {
                        const Point2D& spawn = entityManager.template GetComponent<RespawnComponent>(entityA).loc;
                        // Move the location and the box together by the same offset
                        MoveOutOfCollision(locationA, spawn.x - locationA.loc.x, spawn.y - locationA.loc.y);
                        collisionA.previousLocation = locationA.loc;
                        movementA.velocity = Point2D(0.0f, 0.0f);
                        break;  // stop checking this entity this frame, since it has moved
                    }
                    ResolveCollision(locationA, movementA, collisionA, collisionB, locationB.loc, aIsPlayer ? 0.0f : kStarBounce);
                    continue;
                }
                // Dynamic vs dynamic
                bool bIsPlayer = entityManager.template HasComponent<InputComponent>(entityB);
                if (aIsPlayer && !bIsPlayer) {
                    collected.push_back(entityB);   // player touched a star
                }
                else if (!aIsPlayer && !bIsPlayer && entityB > entityA) {   // star vs star, each pair once
                    if (!entityManager.template HasComponent<LocationComponent>(entityB) || !entityManager.template HasComponent<MovementComponent>(entityB)) {
                        continue;
                    }
                    auto& movementB = entityManager.template GetComponent<MovementComponent>(entityB);
                    ResolveStarStar(locationA, movementA, collisionA, locationB, movementB, collisionB);
                }
            }
            // Sounds for the player: only when a flag newly turns on
            if (aIsPlayer) {
                bool onWall = collisionA.onLeftWall || collisionA.onRightWall;
                // If the player has just landed on the ground or hit a wall, trigger a camera shake effect
                bool newGround = collisionA.onGround && !playerWasOnGround;
                bool newWall   = onWall && !playerWasOnWall;

                if (newGround || newWall || collisionA.onCeiling) {
                    const float kShakeScale = 0.3f;   
                    // offset: if the player has just hit a wall, shake horizontally based on the previous horizontal velocity; 
                    // if the player has just landed on the ground or hit the ceiling, shake vertically based on the previous vertical velocity
                    Point2D offset(newWall ? velBefore.x * kShakeScale : 0.0f,
                                (newGround || collisionA.onCeiling) ? velBefore.y * kShakeScale : 0.0f);
                    auto& shakeStorage = entityManager.template GetComponentStorage<CameraShake>();
                    for (size_t i = 0; i < shakeStorage.mStorage.size(); ++i) {
                        if (shakeStorage.mValid[i]) {
                            shakeStorage.mStorage[i].frameLimit = 10;
                            shakeStorage.mStorage[i].offset = offset;
                        }
                    }
                }

                // if the player was not on the ground last frame but is now, play the landing sound
                if (collisionA.onGround && !playerWasOnGround) {
                    engine->GetSoundManager().GetSound("land").play();
                }
                // and if the player was not on a wall last frame but is now, play the wall sound
                // Wall slide sound: plays when the player starts sliding, even if they hit the wall while rising
                InputComponent& input = entityManager.template GetComponent<InputComponent>(entityA);
                // pushing intoWall is true if the player is pressing into the wall they are touching
                bool pushingIntoWall = (collisionA.onLeftWall && input.keydown.test(Key::Left)) ||
                                       (collisionA.onRightWall && input.keydown.test(Key::Right));
                // wall sliding if the player is pushing into a wall, not on the ground, and moving downward
                bool wallSliding = pushingIntoWall && !collisionA.onGround && movementA.velocity.y > 0;
                if (wallSliding && !mWasWallSliding) {
                    engine->GetSoundManager().GetSound("wall").play();
                }
                mWasWallSliding = wallSliding;
            }
        }
        // 4: Collect stars after iterating, so we never remove an entity mid-loop
        for (EntityID star : collected) {
            if (!entityManager.IsValid(star)) {
                continue;
            }
            entityManager.RemoveEntity(star);
            auto& scoreStorage = entityManager.template GetComponentStorage<ScoreComponent>();
            for (size_t i = 0; i < scoreStorage.mStorage.size(); ++i) {
                if (scoreStorage.mValid[i]) {
                    scoreStorage.mStorage[i].score = std::min(scoreStorage.mStorage[i].score + 1, 999);
                }
            }
            engine->GetSoundManager().GetSound("star").play();
        }
        return kContinue; 
    }
    private:
    bool mWasWallSliding = false;   // was the player sliding down a wall last frame (for the wall sound)

};

}  // namespace ECSEngine
