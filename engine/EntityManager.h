#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <limits>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "Component.h"
#include "Entity.h"

namespace ECSEngine {

template <typename T, typename... Types>
struct ComponentTypeIndex;

template <typename T, typename... Rest>
struct ComponentTypeIndex<T, T, Rest...> : std::integral_constant<size_t, 0> {};

template <typename T, typename First, typename... Rest>
struct ComponentTypeIndex<T, First, Rest...>
    : std::integral_constant<size_t, 1 + ComponentTypeIndex<T, Rest...>::value> {};

template <typename... Components>
class EntityManager {
public:
    static constexpr size_t kNullComponent = std::numeric_limits<size_t>::max();
    using tEntity = std::vector<Entity>;

    // Raw iterators over ALL slots (including invalid ones)
    // Use IsValid(id) to skip gaps manually
    using const_iterator = tEntity::const_iterator;
    const_iterator cbegin() const { return mEntities.cbegin(); }
    const_iterator cend() const { return mEntities.cend(); }
    const_iterator begin() { return mEntities.begin(); }
    const_iterator end() { return mEntities.end(); }
    const_iterator begin() const { return mEntities.cbegin(); }
    const_iterator end() const { return mEntities.cend(); }

    // Entity queries
    bool IsValid(EntityID entity) const {
        return entity < mEntityValid.size() && mEntityValid[entity];
    }

    size_t EntityCount() const { return mEntities.size() - mFreeEntities.size(); }


    [[nodiscard]] EntityID CreateEntity(const std::string& name) { 
        EntityID id;
        if (!mFreeEntities.empty()) {
            // Reuse a previously removed entity ID
            id = mFreeEntities.back(); 
            mFreeEntities.pop_back(); // remove id from the free list

            mEntities[id] = Entity(id, name);
            mEntityValid[id] = true;
            // When an entity is created, it initially has no components
            // all its component indices should start as kNullComponent
            mEntityComponentIndices[id].fill(kNullComponent);
        } else {
            // Create a new entity ID
            id = mEntities.size();

            mEntities.push_back(Entity(id, name));
            mEntityValid.push_back(true);

            std::array<size_t, sizeof...(Components)> indices;
            indices.fill(kNullComponent);
            // New row for the new entity ID
            mEntityComponentIndices.push_back(indices);
        }

        return id; }

    void RemoveEntity(EntityID entity) {
        // Check if the enity exists and valid
        assert(IsValid(entity));
        // Remove all components belonging to this entity
        (RemoveComponentIfPresent<Components>(entity), ...);
        // Mark the entity as invalid
        mEntityValid[entity] = false;

        // Make its ID available for reuse
        mFreeEntities.push_back(entity);
    }

    template <typename T>
    void AddComponent(EntityID entity, T component) {
        // check that the entity is valid and doesn't already have the component
        static_assert(IsOneOf<T, Components...>, "Component type is not registered with this EntityManager");
        assert(IsValid(entity));
        assert(!HasComponent<T>(entity));

        // Find the mapping column
        constexpr size_t typeIndex = ComponentTypeIndex<T, Components...>::value;
        auto& storage = GetComponentStorage<T>(); // Retrieve the appropriate storage
        size_t componentIndex;
        if (!storage.mFreeList.empty()) {
            // Reuse an available component slot
            componentIndex = storage.mFreeList.back();
            storage.mFreeList.pop_back();

            storage.mStorage[componentIndex] = std::move(component);
            storage.mValid[componentIndex] = true;
        } else {
        // Create a new component slot
        componentIndex = storage.mStorage.size();

        storage.mStorage.push_back(std::move(component));
        storage.mValid.push_back(true);
        }
        // Remember where this entity's component is stored
        mEntityComponentIndices[entity][typeIndex] = componentIndex;
    }

    template <typename T>
    void RemoveComponent(EntityID entity) {
        // Check if the entity exists and valid
        static_assert(IsOneOf<T, Components...>, "Component type is not registered with this EntityManager");
        assert(IsValid(entity));
        // Check if the entity has the component
        assert(HasComponent<T>(entity));
        constexpr size_t typeIndex = ComponentTypeIndex<T, Components...>::value;
        // Find where this component is stored
        size_t componentIndex = mEntityComponentIndices[entity][typeIndex];
        // Get the storage for this component type
        auto& storage = GetComponentStorage<T>();
        // Mark the component as invalid
        storage.mValid[componentIndex] = false;
        // Make this storage slot available for reuse
        storage.mFreeList.push_back(componentIndex);
        // Remove the mapping between this entity and the component
        mEntityComponentIndices[entity][typeIndex] = kNullComponent;
    }

    template <typename T>
    bool HasComponent(EntityID entity) const {
        static_assert(IsOneOf<T, Components...>, "Component type is not registered with this EntityManager");
        assert(IsValid(entity));
        // Find the array column for the requested type
        constexpr size_t typeIndex = ComponentTypeIndex<T, Components...>::value; 
        size_t componentIndex = mEntityComponentIndices[entity][typeIndex]; // Read the component index
        return componentIndex != kNullComponent;
    }

    template <typename T>
    T& GetComponent(EntityID entity) {
        assert(HasComponent<T>(entity)); // Checks that entity has the component
        constexpr size_t typeIndex = ComponentTypeIndex<T, Components...>::value; 
        size_t componentIndex = mEntityComponentIndices[entity][typeIndex]; // Finds the component's storage index
        // Gets the component storage
        auto& storage = GetComponentStorage<T>();
        return storage.mStorage[componentIndex]; // return the reference
    }

    template <typename T>
    ComponentStorage<T>& GetComponentStorage() {
        static_assert(IsOneOf<T, Components...>, "Component type is not registered with this EntityManager");
        return std::get<ComponentStorage<T>>(mComponentStorages);
    }
    template <typename T>
    const ComponentStorage<T>& GetComponentStorage() const {
        static_assert(IsOneOf<T, Components...>, "Component type is not registered with this EntityManager");
        return std::get<ComponentStorage<T>>(mComponentStorages);
    }

private:
    tEntity mEntities; // Stores the actual Entity objects
    std::vector<bool> mEntityValid; // Tracks whether an entity ID is currently valid
    std::vector<size_t> mFreeEntities; // Keeps track of entity IDs we can reuse
    // Each value of mEntityComponentIndices is the corresponding component's storage index
    std::vector<std::array<size_t, sizeof...(Components)>> mEntityComponentIndices;
    std::tuple<ComponentStorage<Components>...> mComponentStorages; // creates one component storage for each supported type
    // Helper: checks whether an entity has a particular component before trying to remove it
    template <typename T>
    void RemoveComponentIfPresent(EntityID entity) {
        if (HasComponent<T>(entity)) {
            RemoveComponent<T>(entity);
        }
    }
};

}  // namespace ECSEngine
