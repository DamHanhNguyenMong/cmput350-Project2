#include "SoundManager.h"

#include <cassert>
#include <stdexcept>
#include <utility>

namespace ECSEngine {

void SoundManager::RegisterSound(const std::string& soundPath, const std::string& soundName) {
    if (mSounds.count(soundName) > 0) {
        throw std::logic_error("Sound already registered: " + soundName);
    }
    // Create an empty buffer that will hold the audio data
    sf::SoundBuffer buffer;
    // Load audio file
    if (!buffer.loadFromFile(soundPath)) { // load the audio file from the given path
        throw std::runtime_error("Failed to load sound: " + soundPath);
    }
    // Store the buffer and create the sound
    auto result = mSoundBuffers.emplace(soundName, std::move(buffer)); // stores the buffer in our map using soundName as the key.
    sf::Sound sound(result.first->second); // creates an sf::Sound that references the buffer we just stored
    mSounds.emplace(soundName, std::move(sound)); // stores the sound in our map using soundName as the key

}

sf::Sound& SoundManager::GetSound(const std::string& soundName) {
    // Find the sound in the map, it stores the sound objects that can be played
    auto it = mSounds.find(soundName);
    // catches attempts to retrieve a sound that hasn't been registered
    assert(it != mSounds.end());
    // Reference to the actual stored sound
    return it->second;
}

}  // namespace ECSEngine
