// In SoundManager.cpp

#include "SoundManager.h"
#include "spdlog/spdlog.h"

namespace enDjinn {

    // Constructor and Destructor are unchanged
    SoundManager::SoundManager(ResourceManager& resourceManager)
        : m_resourceManager(resourceManager)
    {
        spdlog::info("SoundManager created.");
    }

    SoundManager::~SoundManager() {
        Shutdown();
    }

	// Startup method to initialize SoLoud
    void SoundManager::Startup() {
        SoLoud::result result = m_soloud.init();
        if (result != SoLoud::SO_NO_ERROR) {
            spdlog::error("SoLoud initialization failed: {}", m_soloud.getErrorString(result));
            m_isInitialized = false;
            return;
        }
        m_isInitialized = true;
        spdlog::info("SoLoud initialized successfully.");
    }

	// Shutdown method to deinitialize SoLoud and clean up sounds
    void SoundManager::Shutdown() {
        if (m_isInitialized) {
            m_soloud.deinit();
            m_isInitialized = false;
            spdlog::info("SoLoud deinitialized.");
        }
        // The unique_ptrs in the map will handle deleting the Wav objects automatically.
        m_sounds.clear();
        spdlog::info("SoundManager shut down.");
    }

    // LoadSound method to load the sounds to play
    bool SoundManager::LoadSound(const std::string& name, const std::string& partialPath) {
        if (!m_isInitialized) return false;

		// Check for existing sound
        if (m_sounds.count(name)) {
            spdlog::warn("Sound with name '{}' already exists, overwriting.", name);
        }

		// Resolve the full path using ResourceManager
        std::filesystem::path fullPath = m_resourceManager.ResolvePath(partialPath);

        // Create a new Wav object on the heap, managed by a unique_ptr.
        auto wav = std::make_unique<SoLoud::Wav>();

        // Load the sound data into the new object.
        SoLoud::result result = wav->load(fullPath.string().c_str());
		// Check for loading errors
        if (result != SoLoud::SO_NO_ERROR) {
            spdlog::error("Failed to load sound '{}' from '{}': {}", name, fullPath.string(), m_soloud.getErrorString(result));
            return false;
        }

        // Move the unique_ptr (ownership of the Wav object) into the map.
        m_sounds[name] = std::move(wav);
        spdlog::info("Sound '{}' loaded successfully.", name);
        return true;
    }

	// DestroySound method to remove sounds from the manager
    void SoundManager::DestroySound(const std::string& name) {
        // Erasing the unique_ptr from the map automatically triggers its destructor,
        // which deletes the managed Wav object.
        if (m_sounds.erase(name) > 0) {
            spdlog::info("Sound '{}' destroyed.", name);
        }
        else {
            spdlog::warn("Attempted to destroy non-existent sound '{}'.", name);
        }
    }

	// PlaySound method to play a loaded sound
    void SoundManager::PlaySound(const std::string& name, float volume, float pan, int loopCount) {
        if (!m_isInitialized) return;

        // Find the sound in the map.
        auto it = m_sounds.find(name);
        if (it != m_sounds.end()) {
            unsigned int handle = m_soloud.play(*(it->second));
            m_soloud.setVolume(handle, volume); // Use per-handle volume for more control
            m_soloud.setPan(handle, pan);
            m_soloud.setLooping(handle, loopCount != 0);
            spdlog::debug("Playing sound '{}'.", name); // Use debug for less log spam
        }
        else {
            spdlog::warn("Attempted to play non-existent sound '{}'.", name);
        }
    }

} // namespace enDjinn