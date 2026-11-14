#pragma once

#include <bitset>
#include <unordered_map>
#include <cmath>

#include "CameraComponent.h"
#include "EntityManager.h"
#include "MathUtil.h"
#include "SpriteManager.h"
#include "SystemManager.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

template <typename... Components>
class SpriteSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("SpriteSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();
        auto& sm = engine->GetSpriteManager();
        auto& wm = engine->GetWindowManager();

        // Go through each entity to check we need to render a sprite for it
        for (const Entity& entity : em) {
            EntityID id = entity.GetID();
            if (!em.IsValid(id)) { continue; }
            if (!em.template HasComponent<SpriteComponent>(id)) { continue; }
            if (!em.template HasComponent<LocationComponent>(id)) { continue; }

            // Entity has a valid sprite component, and a location component
            auto& spriteComponent = em.template GetComponent<SpriteComponent>(id);
            auto& locationComponent = em.template GetComponent<LocationComponent>(id);

            sf::Sprite sprite = sm.GetSprite(spriteComponent.textureID);

            Point2D location(locationComponent.loc.x + spriteComponent.bounds.topLeft.x,
                            locationComponent.loc.y + spriteComponent.bounds.topLeft.y);  // if sprite is not in worldspace
            if (spriteComponent.worldSpace) {
                // Sprite is in world space, need to convert location and scale
                location = wm.WorldToWindow(location);
                float scale = 1.0f / wm.GetWorldScale();
                sprite.setScale({scale, scale});
            }
            location.x = std::round(location.x);
            location.y = std::round(location.y);
            sprite.setPosition({location.x, location.y});

            wm.GetWindow()->draw(sprite);
        }

        return kContinue; 
    }
};

}  // namespace ECSEngine
