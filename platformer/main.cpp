#include <fstream>
#include <string>
#include <iostream>
#include <stdexcept>
#include <unordered_map>

#include "CameraComponent.h"
#include "CameraSystem.h"
#include "CollisionComponent.h"
#include "CollisionLateSystem.h"
#include "CollisionSystem.h"
#include "ECSEngine.h"
#include "EntityManager.h"
#include "GravitySystem.h"
#include "InputComponent.h"
#include "InputSystem.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "MovementSystem.h"
#include "ScoreComponent.h"
#include "ScoreSystem.h"
#include "SpawnComponent.h"
#include "SpawnSystem.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SpriteSystem.h"

std::string gResourcePath = "../assets/";
#define COMPONENTS                                                                             \
    ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent, \
        ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,     \
        ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,     \
        ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::RespawnComponent
using Engine = ECSEngine::ECSEngine<COMPONENTS>;

void LoadMap(Engine& engine, const std::string& path, const std::string& file) {
    std::ifstream in(path + file);
    if (!in) {
        throw std::runtime_error("Cant open map:" + path + file);
    }

    auto& em = engine.GetEntityManager();
    auto& spm = engine.GetSpriteManager();

    // Use a struct so i only need to have one unordered map
    struct Tile {
        ECSEngine::SpriteID sprite;
        ECSEngine::Rect bounds;
    };

    std::unordered_map<char, Tile> dict;

    std::string label;
    int dictType;  // determines decorative or gameplay

    in >> label;  // reads the string label
    in >> dictType;  // reads the int dictType - either 1 or 2

    // Go through dictionary entries until we hit 'map'
    while (in >> label && label != "map") {
        char key = label[0];  // to use as a key in the dict
        std::string texture;
        float sx, sy, sw, sh;  // 'source' x, y, w, h
        Tile tile;

        in >> texture >> sx >> sy >> sw >> sh;

        tile.sprite = spm.RegisterTexture(path + texture, ECSEngine::Rect{{sx, sy}, sw, sh});
        tile.bounds = ECSEngine::Rect{{0, 0}, sw, sh};  // this will be correct for dictType == 1

        // based on type of dict - 2 means gameplay not just decorative
        if (dictType == 2) {
            float bx, by, bw, bh;  // bounds
            in >> bx >> by >> bw >> bh;
            tile.bounds = ECSEngine::Rect{{bx, by}, bw, bh};
        }
        dict[key] = tile;
    }

    // origin x and y, tile width and height, size width and height
    float ox, oy, tw, th;
    int gw, gh;
    in >> label >> ox >> oy;
    in >> label >> tw >> th;
    in >> label >> gw >> gh;

    std::string line;
    std::getline(in, line);  // get rest of header line

    for (int row = 0; row < gh && std::getline(in, line); row++) {
        for (int col = 0; col < gw && col < (int)line.size(); col++) {
            auto iterator = dict.find(line[col]);
            if (iterator == dict.end()) {
                continue;
            }

            const Tile& tile = iterator->second;  // second gets second member

            // set up tile components
            ECSEngine::EntityID id = em.CreateEntity(std::string("Tile-") + line[col]);
            ECSEngine::LocationComponent location{{ox + (col * tw), oy + (row * th)}};
            em.AddComponent(id, location);  // tile location component

            ECSEngine::SpriteComponent sprite;
            sprite.textureID = tile.sprite;
            sprite.bounds = tile.bounds;
            sprite.worldSpace = true;
            em.AddComponent(id, sprite);  // tile sprite component

            if (dictType == 2) {
                // collision stuff
            }
            if (line[col] == 'S') {
                // spawn stuff
            }
        }
    }

}

int main(int argc, char* argv[]) {
    if (argc >= 3 && argv[1] == std::string("-path")) {
        gResourcePath = argv[2];
    }
    std::cout << "Usage: " << argv[0] << " [-path resource_path]\n";
    std::cout << "Using resource path: " << gResourcePath << "\n";

    // Engine test code
    Engine engine(1024, 768, "Test Engine");

    auto& wm = engine.GetWindowManager();
    auto& em = engine.GetEntityManager();
    auto& spm = engine.GetSpriteManager();

    ECSEngine::Rect windowRect = wm.GetWindowBounds();
    float width = windowRect.width;
    float height = windowRect.height;

    // Load sounds

    // Load maps
    LoadMap(engine, gResourcePath, "sky.map");
    LoadMap(engine, gResourcePath, "world.map");

    // Place player
    float playerX = 1200.0f;  // temp variable so setup camera stops yelling at me
    float playerY = 300.0f;  // also a placeholder
    ECSEngine::EntityID playerID = em.CreateEntity("Player");
    em.AddComponent(playerID, ECSEngine::LocationComponent{ECSEngine::Point2D(playerX, playerY)});  // for camera testing

    // Setup score component
    ECSEngine::ScoreComponent scoreComponent;
    scoreComponent.score = 0;

    // Register textures for each digit
    const float digitSide = 64.0f;  // 64x64
    float x = 13 * digitSide;  // digits are in the 14th column (13th index)

    for (int digit = 0; digit < 10; digit++) {
        float y = (13 - digit) * digitSide;

        scoreComponent.digitTextureID[digit] = spm.RegisterTexture(
            gResourcePath + "spritesheet-tiles-default.png",
            ECSEngine::Rect{{x, y}, digitSide, digitSide}
        );
    }

    // Create digit entities
    const float scoreX = 20.0f;
    const float scoreY = 20.0f;
    for (int i = 0; i < 3; i++) {
        // Add digit location component
        ECSEngine::EntityID digitId = em.CreateEntity("ScoreDigit" + std::to_string(i));
        ECSEngine::LocationComponent location;
        location.loc = ECSEngine::Point2D(scoreX + (i * digitSide), scoreY);
        em.AddComponent(digitId, location);

        // Add digit sprite component
        ECSEngine::SpriteComponent sprite;
        sprite.textureID = scoreComponent.digitTextureID[0];
        sprite.bounds = ECSEngine::Rect{{0.0f, 0.0f}, digitSide, digitSide};
        sprite.worldSpace = false;  // fixed on screen
        em.AddComponent(digitId, sprite);

        scoreComponent.digitID[i] = digitId;
    }

    // Add score entity itself
    ECSEngine::EntityID scoreID = em.CreateEntity("Score");
    em.AddComponent(scoreID, scoreComponent);


    // Setup Camera
    ECSEngine::CameraComponent cameraComponent;
    cameraComponent.position = {playerX, height / 2.0f};  // adjust y position as necessary based on map
    cameraComponent.worldUnitsPerPixel = 1.0f;
    cameraComponent.hardExclusion = ECSEngine::Rect{{width*0.1f, 0.0f}, width*0.8f, height};  // 0.1W to 0.9W
    cameraComponent.softExclusion = ECSEngine::Rect{{width*0.3f, 0.0f}, width*0.4f, height};  // 0.3W to 0.7W
    cameraComponent.speed = 0.1f;

    ECSEngine::CameraFollower cameraFollower{playerID};
    ECSEngine::CameraShake cameraShake{0, {0.0f, 0.0f}};  // frameLimit, offset

    // Add camera entity
    ECSEngine::EntityID cameraID = em.CreateEntity("Camera");
    em.AddComponent(cameraID, cameraComponent);
    em.AddComponent(cameraID, cameraFollower);
    em.AddComponent(cameraID, cameraShake);

    // Add all systems
    auto& sm = engine.GetSystemManager();
    sm.AddSystem(std::make_unique<ECSEngine::CollisionLateSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::InputSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::GravitySystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::MovementSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CollisionSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::ScoreSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CameraSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpriteSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpawnSystem<COMPONENTS>>());

    engine.Run();
    return 0;
}
