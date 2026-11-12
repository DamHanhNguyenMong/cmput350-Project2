#pragma once

#include <algorithm>
#include <string>

#include "CollisionComponent.h"
#include "LocationComponent.h"
#include "SpriteComponent.h"
#include "SystemManager.h"

namespace ECSEngine {

template <typename... Components>
class CollisionLateSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CollisionLateSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override { 
        auto& entityManager = engine->GetEntityManager();

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
            // Save the box before input, gravity, and movement
            collision.previousCollisionBox = collision.collisionBox;
        }
        return kContinue; 
    }
};

}  // namespace ECSEngine
