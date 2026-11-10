#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "ECSEngine.h"
#include "EntityManager.h"
#include "InputComponent.h"

using ECSEngine::EntityManager;
using ECSEngine::EntityID;
using ECSEngine::EngineView;
using ECSEngine::System;
using ECSEngine::tSystemReturn;
using ECSEngine::kExit;
using ECSEngine::kContinue;
using ECSEngine::Rect;
using ECSEngine::Point2D;
using ECSEngine::InputComponent;

// ---------- Part 1: EntityManager tests (no window needed) ----------

struct Position { float x, y; };
struct Health { int hp; };
struct Name { std::string s; };

using Mgr = EntityManager<Position, Health, Name>;

static void TestEntityManager() {
    // Sequential IDs, no components at start
    {
        Mgr m;
        auto a = m.CreateEntity("a");
        auto b = m.CreateEntity("b");
        assert(a == 0 && b == 1);
        assert(m.IsValid(a) && m.IsValid(b));
        assert(!m.HasComponent<Position>(a));
        assert(m.EntityCount() == 2);
    }
    // Add / Get / Has / modify
    {
        Mgr m;
        auto e = m.CreateEntity("e");
        m.AddComponent(e, Position{1, 2});
        assert(m.HasComponent<Position>(e));
        assert(m.GetComponent<Position>(e).x == 1);
        m.GetComponent<Position>(e).x = 5;
        assert(m.GetComponent<Position>(e).x == 5);
        assert(!m.HasComponent<Health>(e));
    }
    // Components are per-entity (no mixing)
    {
        Mgr m;
        auto a = m.CreateEntity("a");
        auto b = m.CreateEntity("b");
        m.AddComponent(a, Health{10});
        m.AddComponent(b, Health{20});
        assert(m.GetComponent<Health>(a).hp == 10);
        assert(m.GetComponent<Health>(b).hp == 20);
    }
    // RemoveComponent and slot reuse
    {
        Mgr m;
        auto a = m.CreateEntity("a");
        auto b = m.CreateEntity("b");
        m.AddComponent(a, Health{10});
        m.RemoveComponent<Health>(a);
        assert(!m.HasComponent<Health>(a));
        assert(m.GetComponentStorage<Health>().mFreeList.size() == 1);
        m.AddComponent(b, Health{20});  // reuses a's slot
        auto& st = m.GetComponentStorage<Health>();
        assert(st.mStorage.size() == 1);
        assert(st.mFreeList.empty());
        assert(m.GetComponent<Health>(b).hp == 20);
    }
    // RemoveEntity frees components, ID is reused, no stale components
    {
        Mgr m;
        auto a = m.CreateEntity("a");
        m.AddComponent(a, Position{1, 1});
        m.AddComponent(a, Health{1});
        m.RemoveEntity(a);
        assert(!m.IsValid(a));
        assert(m.EntityCount() == 0);
        assert(m.GetComponentStorage<Position>().mFreeList.size() == 1);
        assert(m.GetComponentStorage<Health>().mFreeList.size() == 1);
        auto c = m.CreateEntity("c");
        assert(c == a);
        assert(m.IsValid(c));
        assert(!m.HasComponent<Position>(c));
        assert(!m.HasComponent<Health>(c));
    }
    // Iteration with manual skipping of gaps
    {
        Mgr m;
        auto e0 = m.CreateEntity("0");
        auto e1 = m.CreateEntity("1");
        auto e2 = m.CreateEntity("2");
        auto e3 = m.CreateEntity("3");
        m.RemoveEntity(e1);
        m.RemoveEntity(e2);

        std::vector<EntityID> seen;
        for (auto it = m.cbegin(); it != m.cend(); ++it) {
            if (!m.IsValid(it->GetID())) continue;
            seen.push_back(it->GetID());
        }
        assert((seen == std::vector<EntityID>{e0, e3}));

        // Raw iteration still visits every slot
        size_t total = 0;
        for (auto it = m.cbegin(); it != m.cend(); ++it) ++total;
        assert(total == 4);
    }
    // Gaps at the start and the end
    {
        Mgr m;
        auto e0 = m.CreateEntity("0");
        auto e1 = m.CreateEntity("1");
        auto e2 = m.CreateEntity("2");
        m.RemoveEntity(e0);
        m.RemoveEntity(e2);
        size_t valid = 0;
        for (auto it = m.cbegin(); it != m.cend(); ++it)
            if (m.IsValid(it->GetID())) { assert(it->GetID() == e1); ++valid; }
        assert(valid == 1);
    }
    // Empty manager and all removed
    {
        Mgr m;
        assert(m.cbegin() == m.cend());
        auto a = m.CreateEntity("a");
        auto b = m.CreateEntity("b");
        m.RemoveEntity(a);
        m.RemoveEntity(b);
        assert(m.EntityCount() == 0);
        for (auto it = m.cbegin(); it != m.cend(); ++it)
            assert(!m.IsValid(it->GetID()));
    }
    // Non-trivial component + const access
    {
        Mgr m;
        auto e = m.CreateEntity("e");
        m.AddComponent(e, Name{"hello"});
        const Mgr& cm = m;
        assert(cm.HasComponent<Name>(e));
        assert(cm.GetComponentStorage<Name>().mStorage.size() == 1);
        assert(m.GetComponent<Name>(e).s == "hello");
    }
    // Stress: create/remove cycles must not grow storage
    {
        Mgr m;
        for (int round = 0; round < 100; ++round) {
            std::vector<EntityID> ids;
            for (int i = 0; i < 100; ++i) {
                auto id = m.CreateEntity("x");
                m.AddComponent(id, Position{float(i), 0});
                ids.push_back(id);
            }
            for (auto id : ids) m.RemoveEntity(id);
            assert(m.EntityCount() == 0);
        }
        assert(m.GetComponentStorage<Position>().mStorage.size() == 100);
    }
    std::cout << "EntityManager tests passed\n";
}

// ---------- Part 2: engine smoke test (opens a window) ----------

using Engine = ECSEngine::ECSEngine<InputComponent>;
using View = EngineView<InputComponent>;

class TestSystem : public System<InputComponent> {
public:
    tSystemReturn Run(View* engine) override {
        auto& in = engine->GetEntityManager().GetComponent<InputComponent>(mPlayer);

        if (in.keydown != in.lastKeydown)
            std::cout << "frame " << mFrames << ": keys changed\n";

        if (in.keydown.test(static_cast<size_t>(sf::Keyboard::Scan::Escape)))
            return kExit;

        engine->GetWindowManager().DebugDrawRect(
            Rect{Point2D(100, 100), 50.f, 50.f}, 255, 0, 0);

        return ++mFrames >= 1800 ? kExit : kContinue;  // ~30 s at 60 fps
    }
    const std::string& GetName() const override { return mName; }

    EntityID mPlayer = 0;
private:
    int mFrames = 0;
    std::string mName = "TestSystem";
};

static void TestEngine() {
    Engine engine(800, 600, "Engine test");
    auto& em = engine.GetEntityManager();
    auto player = em.CreateEntity("player");
    em.AddComponent(player, InputComponent{player});

    auto sys = std::make_unique<TestSystem>();
    sys->mPlayer = player;
    engine.GetSystemManager().AddSystem(std::move(sys));

    engine.Run();
    std::cout << "Engine exited cleanly\n";
}

int main(int argc, char* argv[]) {
    TestEntityManager();
    if (argc > 1 && std::string(argv[1]) == "--headless") return 0;  // skip window test
    TestEngine();
    return 0;
}