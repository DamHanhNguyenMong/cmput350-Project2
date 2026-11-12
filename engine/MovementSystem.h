#pragma once

#include "LocationComponent.h"
#include "SystemManager.h"

namespace ECSEngine {

template <typename... Components>
class MovementSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("MovementSystem");
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
            // If its not a MovementComponent and LocationComponent, skip it
            if (!entityManager.template HasComponent<MovementComponent>(entity) || !entityManager.template HasComponent<LocationComponent>(entity)) {
                continue;
            }
            auto& movement = entityManager.template GetComponent<MovementComponent>(entity);
            auto& location = entityManager.template GetComponent<LocationComponent>(entity);
            // Update the entity's position based on its velocity and the time step
            location.loc.x += movement.velocity.x;
            location.loc.y += movement.velocity.y;
        }
        return kContinue; 
    }
};

}  // namespace ECSEngine
