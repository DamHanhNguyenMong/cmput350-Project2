#pragma once

#include <cmath>
#include <random>
#include <string>
#include <vector>
#include "CollisionComponent.h"
#include "GravityComponent.h"
#include "LocationComponent.h"
#include "MovementComponent.h"
#include "SpawnComponent.h"
#include "SpriteComponent.h"
#include "SystemManager.h"

namespace ECSEngine {

template <typename... Components>
class SpawnSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("SpawnSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();
        auto& spawnStorage = em.template GetComponentStorage<SpawnComponent>();
        // Store requests first because creating entities can resize storage
        // since adding a component during iteration can invalidate iterators, 
        // we will first collect all spawn requests and then process them after the loop
        struct SpawnRequest {
            std::string type;
            SpriteID textureID;
            Point2D location;
        };
        std::vector<SpawnRequest> requests;
        // Process all spawners
        for (size_t i = 0; i < spawnStorage.mStorage.size(); ++i) {
            SpawnComponent& spawn = spawnStorage.mStorage[i]; // Get the spawn component from storage
            EntityID entity = spawn.entityID;
            if (!spawnStorage.mValid[i]) continue;
            if (!em.IsValid(entity)) {
                continue;
            }
            // if totalSpawns is 0 or less, skip this spawner
            if (spawn.totalSpawns <= 0) {
                continue;
            }
            if (!em.template HasComponent<LocationComponent>(entity)) {
                continue;
            }
            // Decrease the timer
            if (spawn.timeToNextSpawn > 0) {
                --spawn.timeToNextSpawn;
            }
            // Not time to spawn yet
            if (spawn.timeToNextSpawn > 0) {
                continue;
            }
            // Record a spawn request
            LocationComponent& location = em.template GetComponent<LocationComponent>(entity);
            requests.push_back({spawn.spawnType, spawn.spawnTextureID, location.loc});
            // Update the spawner
            --spawn.totalSpawns;
            spawn.timeToNextSpawn = spawn.timeBetweenSpawns; // time to next spawn is reset
        }
        if (requests.empty()) return kContinue;
        // Random number generation
        static std::mt19937 rng(std::random_device{}());

        std::uniform_real_distribution<float> angleDist(0.0f, 6.283185307f); // 0 to 2*pi radians
        std::uniform_real_distribution<float> speedDist(2.0f, 5.0f); // Random speed between 2 and 5 units
        // Create entities after finishing the storage iteration
        for (const SpawnRequest& request : requests) {
            EntityID starID = em.CreateEntity(request.type);
            // 1. Location
            LocationComponent location{request.location};
            em.AddComponent(starID, location);
            // 2. Sprite
            SpriteComponent sprite;
            sprite.textureID = request.textureID;
            sprite.bounds = Rect{{0, 0}, 64, 64};
            sprite.worldSpace = true;
            em.AddComponent(starID, sprite);
            // 3. Random movement
            float angle = angleDist(rng);
            float speed = speedDist(rng);
            MovementComponent movement;
            movement.velocity = Point2D(std::cos(angle) * speed, std::sin(angle) * speed);
            em.AddComponent(starID, movement);
            em.AddComponent(starID, GravityComponent(Point2D(0.0f, 0.5f)));   // lighter than the player's 1.5
            // 4. Collision
            CollisionComponent collision{};
            collision.staticEntity = false;
            collision.doesDamage = false;

            // Initialize collision bounds using the sprite and location
            collision.collisionBox = Rect{{0, 0}, 64, 64};
            collision.previousLocation = request.location;

            em.AddComponent(starID, collision);
        }
        return kContinue; 
    }
};

}  // namespace ECSEngine
