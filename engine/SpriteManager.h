#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

using SpriteID = size_t;

class SpriteManager {
public:
    SpriteManager() = default;
    ~SpriteManager() = default;
    [[nodiscard]] SpriteID RegisterTexture(const std::string& texturePath, const Rect& sourceRect);
    sf::Sprite& GetSprite(SpriteID id);

private:
    std::unordered_map<std::string, sf::Texture> mTextures; // Stores loaded textures, using their file paths as keys
    std::vector<sf::Sprite> mSprites; // Stores sprites created from those textures
};

}  // namespace ECSEngine
