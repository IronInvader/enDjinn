// In SoundManager.h

#pragma once

#include "soloud.h"
#include "soloud_wav.h"
#include "./assets/ResourceManager.h" // Corrected header name

#include <string>
#include <unordered_map>
#include <memory> // <<< ADD THIS for std::unique_ptr

namespace enDjinn {

    class SoundManager {
    public:
        SoundManager(ResourceManager& resourceManager);
        ~SoundManager();

        void Startup();
        void Shutdown();
        bool LoadSound(const std::string& name, const std::string& partialPath);
        void DestroySound(const std::string& name);
        void PlaySound(const std::string& name, float volume = 1.0f, float pan = 0.0f, int loopCount = 0);

    private:
        SoLoud::Soloud m_soloud;
        // --- MODIFIED LINE ---
        // Store unique_ptrs to Wav objects instead of the objects directly.
        std::unordered_map<std::string, std::unique_ptr<SoLoud::Wav>> m_sounds;

        ResourceManager& m_resourceManager;
        bool m_isInitialized = false;
    };

} // namespace enDjinn