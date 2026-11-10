#pragma once

#include <unordered_map>
#include <string>

#include <SFML/Audio.hpp>

namespace ECSEngine {

class SoundManager {
public:
    SoundManager() = default;
    ~SoundManager() = default;
    void RegisterSound(const std::string& soundPath, const std::string& soundName);
    sf::Sound& GetSound(const std::string& soundName);

private:
    std::unordered_map<std::string, sf::SoundBuffer> mSoundBuffers; // Stores the actual audio data loaded from files
    std::unordered_map<std::string, sf::Sound> mSounds; // Stores the sound objects that can be played
};

}  // namespace ECSEngine
