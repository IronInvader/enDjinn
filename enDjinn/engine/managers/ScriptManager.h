#pragma once

// Must be defined before including sol/sol.hpp
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#include "InputManager.h"
#include "../assets/ResourceManager.h"
#include "../utils/Types.h"
#include "SoundManager.h"

namespace enDjinn
{
    class ScriptManager {
    public:
        ScriptManager();
        ~ScriptManager();

        bool Startup();
        void Shutdown();

        sol::state& GetLuaState() { return lua; }

        //Lua methods
        sol::state& GetState() { return lua; }
        void ExposeInputManager(enDjinn::InputManager* inputManager);
        void ExposeResourceManager(enDjinn::ResourceManager* resourceManager);
        void ExposeSoundManager(enDjinn::SoundManager* soundManager);
        void RedirectLuaPrint(sol::variadic_args va);
        bool LoadScript(const std::string& name, const std::string& path);
        sol::protected_function* GetScript(const std::string& name);
        void UpdateScriptSystem(float dt);
    private:

        sol::state lua;
        // Storage for compiled Lua scripts, indexed by a user-defined name
        std::unordered_map<std::string, sol::protected_function> m_loadedScripts;
    };
}
