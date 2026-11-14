#pragma once

#include <unordered_map>

#include "CollisionComponent.h"
#include "CameraComponent.h"
#include "CameraFollowerComponent.h"
#include "CameraShakeComponent.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "SpriteComponent.h"
#include "SystemManager.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

template <typename... Components>
class CameraSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name = "CameraSystem";
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& em = engine->GetEntityManager();
        auto& wm = engine->GetWindowManager();
        float width = wm.GetWindowBounds().width;  // window width

        // Find the camera entity
        EntityID cameraID = 0;
        bool found = false;

        for (const Entity& entity : em) {
            EntityID id = entity.GetID();
            if (!em.IsValid(id)) continue;  // skip removed entities
            if (em.template HasComponent<CameraComponent>(id)) {
                cameraID = id;
                found = true;
                break;
            }
        }
        if (!found) return kContinue;  // if no camera component, skip

        CameraComponent& cam = em.template GetComponent<CameraComponent>(cameraID);
        CameraFollower& follower = em.template GetComponent<CameraFollower>(cameraID);
        CameraShake& shake = em.template GetComponent<CameraShake>(cameraID);
        float scale = cam.worldUnitsPerPixel;

        // Only have to follow x coordinate - y never changes
        Point2D location = em.template GetComponent<LocationComponent>(follower.entityID).loc;
        Rect box = em.template GetComponent<CollisionComponent>(follower.entityID).collisionBox;
        float playerX = location.x + box.topLeft.x + box.width / 2.0f;  // Gets center of body to track player
        float screenX = width / 2.0f + (playerX - cam.position.x) / scale; // mid point + cam relative to player

        float hardL = cam.hardExclusion.topLeft.x;
        float hardR = hardL + cam.hardExclusion.width;
        float softL = cam.softExclusion.topLeft.x;
        float softR = softL + cam.softExclusion.width;

        if (screenX < hardL) {cam.position.x += (screenX - hardL) * scale;}  // snap
        else if (screenX > hardR) {cam.position.x += (screenX - hardR) * scale;}
        else if (screenX < softL) {cam.position.x += cam.speed * (screenX - softL) * scale;}  // tweening
        else if (screenX > softR) {cam.position.x += cam.speed * (screenX - softR) * scale;}

        // Shake
        Point2D newShake(0.0f, 0.0f);
        if (shake.frameLimit > 0) {
            float sign = 1.0f;
            if (shake.frameLimit % 2 == 0) { sign = (-1.0f); }  // To 'jitter' back and forth - switch sign every frame

            newShake = Point2D(shake.offset.x * sign, shake.offset.y * sign);
            shake.offset = Point2D(shake.offset.x*0.8f, shake.offset.y*0.8f);  // to decay
            shake.frameLimit--;
        }

        // Apply changes
        wm.SetWorldScale(scale);
        wm.SetWorldCenter(Point2D(cam.position.x + newShake.x, cam.position.y + newShake.y));
        return kContinue; 
    }
};

}  // namespace ECSEngine
