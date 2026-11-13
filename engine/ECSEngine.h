#pragma once

#include <vector>
#include <string>

#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "ScoreComponent.h"
#include "SoundManager.h"
#include "SpawnComponent.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SystemManager.h"
#include "WindowManager.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

namespace ECSEngine {

template <typename... Components>
class EngineView {
public:
    virtual ~EngineView() = default;

    virtual SoundManager& GetSoundManager() = 0;
    virtual SpriteManager& GetSpriteManager() = 0;
    virtual EntityManager<Components...>& GetEntityManager() = 0;
    virtual WindowManager& GetWindowManager() = 0;
};

template <typename... Components>
class ECSEngine : public EngineView<Components...> {
public:
    ECSEngine(unsigned int width, unsigned int height, const std::string& name);
    ~ECSEngine();

    void Run();

    SoundManager& GetSoundManager() override { return mSoundManager; }

    SpriteManager& GetSpriteManager() override { return mSpriteManager; }

    EntityManager<Components...>& GetEntityManager() override { return mEntityManager; }

    WindowManager& GetWindowManager() override { return mWindowManager; }

    SystemManager<Components...>& GetSystemManager() { return mSystemManager; }

private:
    bool ProcessEvents();
    void DebugDraw();

    WindowManager mWindowManager;
    EntityManager<Components...> mEntityManager;
    SystemManager<Components...> mSystemManager;
    SpriteManager mSpriteManager;
    SoundManager mSoundManager;
    

    // For debugging - pause and step the engine
    bool running, step;
    int debug;
};

template <typename... Components>
ECSEngine<Components...>::ECSEngine(unsigned int width, unsigned int height,
                                    const std::string& name)
    : mWindowManager(width, height, name), running(true), step(false), debug(0) {
        auto window = mWindowManager.GetWindow();
        window->setKeyRepeatEnabled(false);
        window->setFramerateLimit(60);
    }

template <typename... Components>
ECSEngine<Components...>::~ECSEngine() {}

template <typename... Components>
void ECSEngine<Components...>::Run() {
    auto window = mWindowManager.GetWindow();

    while (window->isOpen()) {
        // 1. Process window and keyboard events
        if (!ProcessEvents()) {
            break;
        }

        // 2. Run systems if the game is running  or if we requested a single step
        if (running || step) {
            window->clear();
            if (!mSystemManager.Run(this)) { // SpriteSystem only draws
                window->close();
                break;
            }
            // 3. Display the window contents
            // Save the current frame's keyboard state for the next frame
            if constexpr (IsOneOf<InputComponent, Components...>) {
                auto& storage = mEntityManager.template GetComponentStorage<InputComponent>(); // Get the storage for InputComponent
                for (size_t i = 0; i < storage.mStorage.size(); ++i)
                    if (storage.mValid[i]) {
                        storage.mStorage[i].lastKeydown = storage.mStorage[i].keydown; // lastKeyDown is the same as keydown if we hold the key pressed
                    }
            }
            step = false; // Reset the step flag after processing a single step
            if (debug) DebugDraw();
            window->display();
        }
    }
}

template <typename... Components>
bool ECSEngine<Components...>::ProcessEvents() {
    auto window = mWindowManager.GetWindow();
    auto& storage = mEntityManager.template GetComponentStorage<InputComponent>();

    // 1. Process all pending window events
    while (auto event = window->pollEvent()) {
        // Window closed
        if (event->template is<sf::Event::Closed>()) {
            window->close();
            return false;
        }

        // A keyboard key was pressed
        else if (auto keyPressed = event->template getIf<sf::Event::KeyPressed>()) { 
            size_t index = static_cast<size_t>(keyPressed->scancode); // Get the scancode of the pressed key
            if (keyPressed->scancode == sf::Keyboard::Scan::P) {
                running = !running;
            }
            else if (keyPressed->scancode == sf::Keyboard::Scan::N) {
                if (!running) step = true;
            }
            else if (keyPressed->scancode == sf::Keyboard::Scan::F3) {
                debug = !debug;
            }

            if (index < sf::Keyboard::ScancodeCount) {
                for (size_t i = 0; i < storage.mStorage.size(); ++i) {
                    if (storage.mValid[i]) {
                        storage.mStorage[i].keydown.set(index); // report the key as pressed in the keydown bitset
                    }
                }
            }
        }

        // A keyboard key was released
        else if (auto keyReleased = event->template getIf<sf::Event::KeyReleased>()) {
            size_t index = static_cast<size_t>(keyReleased->scancode);
        
            if (index < sf::Keyboard::ScancodeCount) {
                for (size_t i = 0; i < storage.mStorage.size(); ++i) {
                    if (storage.mValid[i]) {
                        storage.mStorage[i].keydown.reset(index); // report the key as released in the keydown bitset
                    }
                }
            }
        }
    }
    return true;
}

template <typename... Components>
void ECSEngine<Components...>::DebugDraw() {
    // Draw collision rects for all valid entities - with collision and location components
    // if statement ensures an engine with no collision / location components (i.e test engine) wont try to run this
    if constexpr (IsOneOf<CollisionComponent, Components...> && IsOneOf<LocationComponent, Components...>) {
        EntityID id = 0;
        for (auto iterator = mEntityManager.begin(); iterator != mEntityManager.end(); ++iterator, ++id) {
            if (!mEntityManager.IsValid(id)) { continue; }
            if (!mEntityManager.template HasComponent<CollisionComponent>(id)) { continue; }
            if (!mEntityManager.template HasComponent<LocationComponent>(id)) { continue; }

            CollisionComponent& collisionComponent = mEntityManager.template GetComponent<CollisionComponent>(id);
            LocationComponent& locationComponent = mEntityManager.template GetComponent<LocationComponent>(id);
            mWindowManager.DebugFrameRect(WorldBox(collisionComponent.collisionBox, locationComponent.loc), 255, 0, 0, 255);
        }
    }
}

}  // namespace ECSEngine
