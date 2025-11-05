#include <iostream>
#include <vector> // Required for std::vector<Sprite>
#include "Engine.h"
#include "spdlog/spdlog.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp> // Required for glm::vec3, glm::vec2

// Define a static list of all key codes you want to monitor
static const std::vector<int> ALL_MONITORED_KEYS = {
    GLFW_KEY_SPACE,
    GLFW_KEY_W,
    GLFW_KEY_A,
    GLFW_KEY_S,
    GLFW_KEY_D,
    GLFW_KEY_Q,
    // Add any other specific keycodes here
};

int main() {
    enDjinn::Engine engine;
    engine.Startup();

    // The script manager is now updated inside the game loop.
    auto* input_manager = engine.GetInputManager();

    sol::state& lua_state = engine.GetScriptManager()->GetLuaState();

    sol::protected_function master_update_func = lua_state["UpdateAllSystems"];

    if (!master_update_func.valid()) {
        spdlog::error("FATAL: Could not find the 'UpdateAllSystems' function in Lua. Exiting.");
        engine.Shutdown();
        return 1;
    }


    engine.RunGameLoop([&]() {

        const float dt_fixed = 1.0f / 60.0f;
        sol::protected_function_result result = master_update_func(dt_fixed);
        engine.GetGraphicsManager()->Draw();
        });

    engine.Shutdown();
    return 0;
}