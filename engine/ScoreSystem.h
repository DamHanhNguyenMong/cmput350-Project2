#pragma once

#include "ScoreComponent.h"
#include "SpriteComponent.h"

namespace ECSEngine {

template <typename... Components>
class ScoreSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("ScoreSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();

        // Go through all entities and check if they are score components
        for (const Entity& entity : em) {
            EntityID id = entity.GetID();
            if (!em.IsValid(id)) { continue; }
            if (!em.template HasComponent<ScoreComponent>(id)) { continue; }

            // Get score component from valid id
            ScoreComponent& sc = em.template GetComponent<ScoreComponent>(id);

            // Ensure the value of score is within range
            int scoreValue = sc.score;
            if (scoreValue < 0) { scoreValue = 0; }
            if (scoreValue > 999) { scoreValue = 999; }

            // Make a list of ints for each digit - hundreds place, tens place, ones place
            int digits[3] = { scoreValue / 100, (scoreValue / 10) % 10, scoreValue % 10 };

            // for each digit set sprite texture
            for (int i = 0; i < 3; i++) {
                EntityID digitEntity = sc.digitID[i];
                if (!em.IsValid(digitEntity)) { continue; }
                if (!em.template HasComponent<SpriteComponent>(digitEntity)) { continue; }

                // Get valid digit entity
                SpriteComponent& sprite = em.template GetComponent<SpriteComponent>(digitEntity);
                sprite.textureID = sc.digitTextureID[digits[i]];  // set sprite texture
            }
        }

        return kContinue; 
    }
};

}  // namespace ECSEngine
