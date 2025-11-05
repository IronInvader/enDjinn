#pragma once

#include "managers/GraphicsManager.h"
#include "managers/InputManager.h"
#include "assets/ResourceManager.h"
#include "managers/SoundManager.h"
#include "managers/ScriptManager.h"
#include <memory>
#include <functional>
#include <sol/sol.hpp>


namespace enDjinn {

    typedef std::function<void()> UpdateCallback;

    class Engine {
    public:
        Engine();
        ~Engine();

        void Startup();
        void RunGameLoop(const UpdateCallback& callback);
        void Shutdown();

        float GetDeltaTime() const { return m_deltaTime; }

        GraphicsManager* GetGraphicsManager() const;
        InputManager* GetInputManager() const;
        ResourceManager* GetResourceManager() const;
        SoundManager* GetSoundManager() const;
        ScriptManager* GetScriptManager() const;
        void QuitGame();

    private:
        float m_deltaTime = 0.0f; // Stores the time between the last two frames (in seconds)
        uint64_t m_lastTime = 0;

        std::unique_ptr<GraphicsManager> m_graphicsManager;
        std::unique_ptr<InputManager> m_inputManager;
        std::unique_ptr<ResourceManager> m_resourceManager;
        std::unique_ptr<SoundManager> m_soundManager;
		std::unique_ptr<ScriptManager> m_scriptManager;
    };

} // namespace enDjinn