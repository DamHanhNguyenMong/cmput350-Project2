# CMPUT 350 HW 2

### Controls
- Left / Right arrow: run left / right respectively
- Z: jump
- C: dash
- P: pause / unpause
- F3: toggle debug collision boxes

### Engine Overview
- ECSEngine<Components...> in ECSEngine.h owns each of the 5 managers.
- Each system receives EngineView<Components...> which they use to access managers via EngineView accessors
    - For example: engine->GetEntityManager()
    - Systems never access managers through globals or locals in main

- ECSEngine::Run() Frame Loop each frame:
    1. ProcessEvents() polls SFML events
    2. When running or stepping through frames, clears the window and runs every system in order through SystemManager::Run()
    3. Copies keydown into lastKeydown for each InputComponent, so that the next frame can detect newly pressed keys
    4. If debug is on, draws collision boxes
- Note that the window is limited to 60 frames per second, and key repeat is off. Speeds and timers are all 'per-frame'

### Resource Managers
- General rule: The engine owns every manager. A reference to a manager from engine.Get___Manager() is valid for the lifetime of the engine object.
- Systems are held by std::unique_ptrs and the window is held by a std::shared_ptr
- Managers are destroyed in reverse order of declaration

#### EntityManager
- Stores one ComponentStorage<T> per component type in a std::tuple
- Each ComponentStorage holds:
    - The packed component data in mStorage
    - mValid to determine whether each slot is in use
    - mFreeList to holds slots that can be reused
- Each entity has a row in mEntityComponentIndicies - one index per component type - or kNullComponent if the entity doesn't have that component
- Entities are stored in a std::vector<Entity> mEntities of which EntityID is the index into that vector
- mEntityValid marks live entities

**API**
- EntityID CreateEntity(name): Returns a new ID, reusing the ID of a removed entity if there is one available. The new entity has no components.
- void RemoveEntity(id): Removes all of the entity's components, marks the ID invalid, and puts the ID on the free list for reuse.
- void AddComponent<T>(id, c): Copies / moves c into storage, reusing a free slot if possible. Asserts the entity is valid and doesn't already have a component of type T.
- void RemoveComponent<T>(id): Checks the entity and component exist, then marks the component slot free.
- bool HasComponent<T>(id): Checks if the entity has a component of type T and returns true if it does - false if not.
- T& GetComponent<T>(id): Returns a reference to the entity's component of type T, first asserting that that component exists
- ComponentStorage<T>& GetComponentStorage<T>(): Direct access to all component types - to be used by systems that only need to visit a particular component type. Skips slots where mValid[i] is false.
- bool IsValid(id): Returns true if the ID refers to a live entity, false otherwise.
- size_t EntityCount(): Returns number of live entities.
- begin(), eng(), cbegin(), cend(): To iterate over every slot in mEntities, including removed ones. A caller will have to skip gaps with IsValid(iterator->GetID())

Lifetime / Validity Rules
- EntityIDs: valid until RemoveEntity, after which the id can be assigned to a different entity.
- T& from GetComponent<T>: valid until the next AddComponent<T> of the same type T, on any entity, since the vector might rellocate. Also invalid when the component or its entity is removed, though like the ids, the slot may be reused.
- ComponentStorage<T>&: the storage object is valid as long as EntityManager is, but references follow the same rules as GetComponent (point above)
- Entity Iterators: valid until CreateEntity is called as the vector may grow. Note that systems should never create entities while iterating.


#### SoundManager
- Stores an unordered_map<string, sf::SoundBuffer> for audio data, keyed by the name given at registration.
- Stores an unordered_map<string, sf::Sound> for playable sounds, keyed by the name given at registration.
  
**API**
- void RegisterSound(path, name): Loads the sound file into a buffer and creates a sound that plays it.
- sf::Sound& GetSound(name): Returns the sound for the given name. Caller can then call .play() on the sound.

Lifetime / Validity Rules
- unordered_map never moves its elements, so a reference from GetSound is valid for the lifetime of the SoundManager, and thus the engine.
- Each sf::Sound points at its buffer. Since the buffers map is declared before sounds map, it is destroyed after all the sounds that use it are destroyed.
- There is only one sf::Sound per name, so playing a sound again restarts it.


#### SpriteManager
- Stores an unordered_map<string, sf::Texture> keyed by the file path.
- Stores a vector<sf::Sprite> where the SpriteID is the index.

**API**
- SpriteID RegisterTexture(path, sourceRect): Loads the texture if it is not already loaded, then creates a sprite using sourceRect of that texture, and returns its ID.
- sf::Sprite& GetSprite(id): Returns the sprite for the given ID, asserting that the ID exists.

Lifetime / Validity Rules
- SpriteIDs are valid for the lifetime of the SpriteManager.
- Sprites and textures are never unloaded until the engine is destroyed.
- Textures live in an unordered_map, so their addresses never change. Each sf::Sprite can thus safely hold a pointer to its texture.
- The reference returned by GetSprite is valid until the next RegisterTexture call, since the vector may reallocate. Callers should copy the sprite if needed more than just immediately.


#### WindowManager
- Owns the sf::RenderWindow.
- The camera is stored as the world point at the center of the window (mWorldCenter) and a scale (mWorldUnitsPerPixel).

**API**
- std::shared_ptr<sf::RenderWindow> GetWindow(): Returns a shared pointer to the window.
- Rect GetWindowBounds(): Returns a rect with topLeft (0,0) and width and height of the window in pixels.
- void SetWorldCenter(pt): Sets the world center point to the given point.
- void SetWorldScale(unitsPerPixel): Sets the world scale (map world coordinates to window pixels)
- void SetCamera(worldPt, screenPt): Moves the center so that worldPt appears at screenPt, keeping current scale.
- void SetWorldRect(rect): Sets the whole world rect, where center of the world is the rect center and scale is the ratio of the two axes.
- GetWorldCenter(), GetWorldScale(): Returns the currently saved world center point / world scale respectively.
- WorldToWindow / WindowToWorld: Convers the given float / point / rect from the first coordinate system to the second (i.e. WorldToWindow converts from world points to window points).
- DebugDrawRect / DebugFrameRect: Draws a filled in / outlined world-space rectangle for the given rect in the given colour.

Lifetime / Validity Rules:
- All return values are plain values except GetWindow.
- The window returned by GetWindow stays alive as long as any copy of the pointer exists.


#### SystemManager
- Stores a vector<unique_ptr<System<Components...>>>

**API**
- void AddSystem(unique_ptr<System>&&): Takes ownership of system. Systems run in the order they were added.
- bool Run(EngineView*): Runs every system once, and returns true. Returns false if any system requests to exit.
- void ClearSystems(): Destroys all systems.

- Systems may keep their own private state between frames


### Components
- All positions are in world pixels.
- Note: A LocationComponent is the top left point of an entity, and sprite and collision boxes are stored relative to it, so moving location moves everything.

- LocationComponent:
  - Point2D loc: base location of the entity.
- MovementComponent:
  - Point2D velocity: velocity of entity in pixels per frame.
- GravityComponent:
  - Point2D acceleration: acceleration to be added to velocity each frame.
  - int disabledTimer: frame to skip gravity - used for dash.
- RespawnComponent:
  - Point2D loc: location where the player is moved after touching spikes. 
- CollisionComponent:
  - uint8_t staticEntity
  - uint8_t doesDamage: for spikes.
  - uint8_t onGround
  - uint8_t onCeiling
  - uint8_t onLeftWall
  - uint8_t onRightWall
  - Rect collisionBox: The bounding box for collision detection - relative to location.
  - Point2D previousLocation: location at the start of the frame to work out which side was hit.
- SpriteComponent:
  - SpriteID textureID
  - Rect bounds: relative drawing rect
  - bool worldSpace: if false, drawn in screen pixels, if true, drawn in world space
- InputComponent:
  - EntityID entity 
  - int lastDash: remaining dash cooldown in frames.
  - int dashTimer: remaining dash duration in frames.
  - int wallJumpTimer: frames left where horizontal input is ignored after a wall jump
  - int wallGraceTimer: frames left where a wall jump is still allowed after leaving a wall
  - int wallSide: which side of the wall player was last on: -1 for left, 1 for right
  - std::bitset<sf::Keyboard::ScancodeCount> keydown
  - std::bitset<sf::Keyboard::ScancodeCount> lastKeydown
- CameraComponent:
  - Point2D position: world point at screen center.
  - float worldUnitsPerPixel
  - Rect hardExclusion: zone outside of which camera is not allowed to go.
  - Rect softExclusion: zone outside of which camera can go but will scroll out of.
  - float speed: tween speed.
- CameraFollower:
  - EntityID entityID: the entity id the camera follows. 
- CameraShake:
  - int frameLimit: number of frames of shake left.
  - Point2D offset: shake 'size' in x and y.
- ScoreComponent:
  - int score
  - SpriteID digitTextureID[10]: sprites for digits 0 to 9.
  - EntityID digitID[3]: the three entities that show the score.
- SpawnComponent:
  - EntityID entityID: the entity this spawn component is attached to.
  - std::string spawnType
  - SpriteID spawnTextureID
  - int timeToNextSpawn: measured in frames.
  - int timeBetweenSpawns: measured in frames.
  - int totalSpawns: spawns left (i.e. if totalSpawns is 10 the entity will spawn 10 more things total)


### Systems - In Run Order
- CollisionLateSystem (CollisionSystemUpdate):
  - Copies each colliding entity's location into CollisionComponent::previousLocation before anything moves.
- InputSystem:
  - Turns keys into action: running, jumping, wall jumps, variable jump height, dash
  - Uses last frames collision flags (on ground / on wall) 
- GravitySystem:
  - Impacts entities with both gravity and movement.
  - Adds gravity when falling, stops vertical velocity on the ground / ceiling, and caps falling speed (lower cap when sliding down a wall)
- MovementSystem:
  - Adds velocity to locations to move entities. 
- CollisionSystem:
  - Clears collision flags, then tests dynamic entities against all other entities:
    - player vs. static entity: set side flag, and if entity is spikes, respawn the player. 
    - star vs. static entity: same but reverse velocity and scale it down by kStarBounce so that stars lose speed each bounce.
    - star vs. star: pushed apart along the overlap. Velocities on that axis are swapped and scaled by kStarBounce.
    - player vs. star: star is collected (removed) and score is incremented.
  - Also triggers screen shake, landing sound, and wall slide sound for the player.
- ScoreSystem:
  - Sets the three digit entities' sprites based on the current score (between 0 and 999) 
- CameraSystem:
  -  Moves the camera in x to follow the player and applies shake, then updates the WindowManager.
- SpriteSystem:
  -  Draws every entity with a sprite and location, converting world-space sprites through the WindowManager.
  -  The engine calls display() after the systems.
- SpawnSystem:
  - Counts down each spawner and records spawn requests, then creates the stars after the loop.
