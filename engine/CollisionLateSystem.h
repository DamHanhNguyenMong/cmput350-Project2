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
            if (!entityManager.template HasComponent<CollisionComponent>(entity) || !entityManager.template HasComponent<LocationComponent>(entity))  {
                continue;
            }
            // Store the current location as the previous location for the next frame
            entityManager.template GetComponent<CollisionComponent>(entity).previousLocation = entityManager.template GetComponent<LocationComponent>(entity).loc;
        }
        return kContinue; 
    }
};

}  // namespace ECSEngine
