#include "SpriteManager.h"

#include <cassert>
#include <stdexcept>
#include <utility>

namespace ECSEngine {

SpriteID SpriteManager::RegisterTexture(const std::string& texturePath, const Rect& sourceRect) {
    // Check whether the texture is already loaded
    auto it = mTextures.find(texturePath);
    // Load the texture if it doesn't exist
    if (it == mTextures.end()) {
        sf::Texture texture; // Create a new texture object
        // Load the texture from the specified file path
        if (!texture.loadFromFile(texturePath)) {
            throw std::runtime_error("Failed to load texture: " + texturePath);
        }
        // We only load each texture once. emplace() inserts it into the map
        auto result = mTextures.emplace(texturePath, std::move(texture));
        it = result.first; // it stores the iterator to the newly inserted texture   
    }
    // Create a sprite using the texture
    sf::Sprite sprite(it->second);
    // Set the source rectangle
    sprite.setTextureRect(sf::IntRect(
        {static_cast<int>(sourceRect.topLeft.x), static_cast<int>(sourceRect.topLeft.y)},
        {static_cast<int>(sourceRect.width), static_cast<int>(sourceRect.height)}
    ));
    // Store the sprite and return its ID (which is the index in the mSprites vector)
    SpriteID id = mSprites.size();
    mSprites.push_back(std::move(sprite));

    return id;

}

sf::Sprite& SpriteManager::GetSprite(SpriteID id) {
    // Check that the ID is valid
    assert(id < mSprites.size());
    // Return the sprite at that index. Return a refernce so other parts can modify it directly
    return mSprites[id];
}

}  // namespace ECSEngine
