
#include "InputManager.h"
#include "../utils/Types.h"
#include "../assets/Sprite.h"
#include "../assets/ResourceManager.h"
#include "ScriptManager.h"
#include "spdlog/spdlog.h"

using namespace enDjinn;

ScriptManager::ScriptManager() = default;
ScriptManager::~ScriptManager() = default;

// Initialize the Lua state and expose C++ types/functions to Lua
bool ScriptManager::Startup() {
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table);
    lua.script("math.randomseed(0)");
    sol::state& lua = GetLuaState();

    // Expose glm::vec3 as 'vec3'
    lua.new_usertype<glm::vec3>("vec3",
        sol::constructors<glm::vec3(float, float, float)>(),
        "x", &glm::vec3::x,
        "y", &glm::vec3::y,
        "z", &glm::vec3::z
    );

    // Expose enDjinn::Sprite as 'Sprite'
    lua.new_usertype<enDjinn::Sprite>("Sprite",
        sol::constructors<enDjinn::Sprite()>(),
        "textureName", &enDjinn::Sprite::textureName,
        "position", &enDjinn::Sprite::position, // This uses the exposed vec3
        "scale", &enDjinn::Sprite::scale,      // Assuming scale is glm::vec2/vec3
        "z", &enDjinn::Sprite::z               // If 'z' is separate
    );

	// Expose glm::vec2 as 'vec2'
    lua.new_usertype<glm::vec2>("vec2",
        sol::constructors<glm::vec2(float, float)>(),
        "x", &glm::vec2::x,
        "y", &glm::vec2::y
    );

	// Explose glm::vec3 again for ScriptComponent (if needed if Engine is supported later)
    lua.new_usertype<glm::vec3>("vec3",
        sol::constructors<glm::vec3(float, float, float)>(),
        "x", &glm::vec3::x,
        "y", &glm::vec3::y,
        "z", &glm::vec3::z
    );

	// Expose enDjinn::ScriptComponent as 'script'
	// VERY IMPORTANT: This must match the C++ ScriptComponent definition
	// The lua name "script" is lowercase to match Lua conventions
    lua.new_usertype<enDjinn::ScriptComponent>("script",
        sol::constructors<enDjinn::ScriptComponent()>(),
        "name", &enDjinn::ScriptComponent::name
    );

	// Redirect Lua's print function to our custom C++ function
	// For debugging purposes
    lua.set_function("print",
        [this](sol::variadic_args va) {
            RedirectLuaPrint(va);
        }
    );

    return true;
}

// Shutdown the Lua state
// Note: sol::state destructor handles Lua state cleanup, so not explicitly needed here
void ScriptManager::Shutdown() {
    // Lua state is typically destroyed when the sol::state object is destroyed.
}

// Expose InputManager functionality to Lua
void ScriptManager::ExposeInputManager(InputManager* inputManager) {
    if (!inputManager) {
        spdlog::error("ScriptManager: Cannot expose InputManager, pointer is null.");
        return;
    }

    // 1. Expose KEYBOARD enum constants
    lua.new_enum<int>("KEYBOARD", {
        { "SPACE", KeyCode::KEY_SPACE },
        { "W", KeyCode::KEY_W },
        { "A", KeyCode::KEY_A },
        { "S", KeyCode::KEY_S },
        { "D", KeyCode::KEY_D },
        { "ESCAPE", KeyCode::KEY_ESCAPE },
        { "LEFT_SHIFT", KeyCode::KEY_LEFT_SHIFT },
		{ "ENTER", KeyCode::KEY_ENTER },
        // ... (More if needed can go be added here)
        });

    // 2. Expose IsKeyPressed function
    // We bind a C++ lambda to the Lua name "IsKeyPressed".
    lua.set_function("IsKeyPressed", [inputManager](const int keycode) {

        // 1. Call the actual C++ function
        bool is_pressed = inputManager->IsKeyPressed(keycode);

        // 2. Log everything we know
        // We only need to see it when we press a key like 'W'.
        // Note: This is not needed, but practical for debugging
        if (is_pressed) {
            spdlog::info("[LUA BINDING] IsKeyPressed called. keycode: {}, result: {}", keycode, is_pressed);
        }

        // 3. Return the result to Lua
        return is_pressed;
        });
	// Log the successful exposure
    spdlog::info("ScriptManager: InputManager exposed to Lua (IsKeyPressed, KEYBOARD enum).");
}

// Expose ResourceManager functionality to Lua
void ScriptManager::ExposeResourceManager(ResourceManager* resourceManager) {
    if (!resourceManager) {
        spdlog::error("ScriptManager: Cannot expose ResourceManager, pointer is null.");
        return;
    }

    // Bind the C++ function to the global Lua name "ResourceManager_LoadImage"
    lua.set_function("ResourceManager_LoadImage",
        [resourceManager](const std::string& name, const std::string& path) {
            bool success = resourceManager->LoadTexture(name, path);

			// Log the result
			// Not needed, but useful for debugging Lua scripts
            if (success) {
                spdlog::info("[LUA]: Loaded image asset '{}' from path '{}'.", name, path);
            }
            else {
                spdlog::error("[LUA]: Failed to load image asset '{}' from path '{}'.", name, path);
            }
        }
    );

	// Log the successful exposure
    spdlog::info("ScriptManager: ResourceManager exposed to Lua (ResourceManager_LoadImage).");
}

// Expose SoundManager functionality to Lua
void ScriptManager::ExposeSoundManager(SoundManager* soundManager) {
    if (!soundManager) {
        spdlog::error("ScriptManager: Cannot expose SoundManager, pointer is null.");
        return;
    }

    // 1. LoadSound Binding
    // Lua function: SoundManager_LoadSound(name, path)
    lua.set_function("SoundManager_LoadSound",
        [soundManager](const std::string& name, const std::string& path) {
            bool success = soundManager->LoadSound(name, path);

			// Log the result
			// Not needed, but useful for debugging Lua scripts
            if (success) {
                spdlog::info("[LUA]: Loaded sound asset '{}' from path '{}'.", name, path);
            }
            else {
                spdlog::error("[LUA]: Failed to load sound asset '{}' from path '{}'.", name, path);
            }
            return success;
        }
    );

    // 2. PlaySound Binding
    // Lua function: SoundManager_PlaySound(name, volume, pan, loopCount)
    // sol will automatically handle the default C++ values for volume, pan, and loopCount
    lua.set_function("SoundManager_PlaySound",
        // We use sol::overload to allow sol to choose the best C++ overload.
        sol::overload(
            // Bind the C++ member function to the SoundManager instance
            [soundManager](const std::string& name, float volume, float pan, int loopCount) {
                soundManager->PlaySound(name, volume, pan, loopCount);
            },
            [soundManager](const std::string& name, float volume, float pan) {
                soundManager->PlaySound(name, volume, pan);
            },
            [soundManager](const std::string& name, float volume) {
                soundManager->PlaySound(name, volume);
            },
            [soundManager](const std::string& name) {
                soundManager->PlaySound(name);
            }
        )
    );

	// Log the successful exposure
    spdlog::info("ScriptManager: SoundManager exposed to Lua (LoadSound, PlaySound).");
}

bool ScriptManager::LoadScript(const std::string& name, const std::string& path) {
    if (m_loadedScripts.count(name)) {
        spdlog::warn("ScriptManager: Script with name '{}' is already loaded.", name);
        return true;
    }

    // 1. Load and compile the script file
    sol::load_result loadResult = lua.load_file(path);

    if (!loadResult.valid()) {
        // Check for errors during compilation/loading
        sol::error err = loadResult;
        spdlog::error("ScriptManager: Failed to load script '{}' from path '{}'. Error: {}",
            name, path, err.what());
        return false;
    }

    // 2. Store the compiled script (which is a sol::protected_function)
    // The load_result can be implicitly converted to a protected_function if valid.
    m_loadedScripts[name] = loadResult;

    spdlog::info("ScriptManager: Successfully loaded and compiled script '{}'.", name);
    return true;
}

void ScriptManager::RedirectLuaPrint(sol::variadic_args va) {
    std::string message;

    // Iterate over all arguments passed to print
    for (auto arg : va) {
        // Use sol::utility::to_string to safely convert Lua types to string
        message += arg.get<sol::object>().as<std::string>();
        message += " "; // Add a space between arguments
    }

    // Log the message using your engine's logger (spdlog)
    spdlog::info("[LUA]: {}", message);
}

sol::protected_function* ScriptManager::GetScript(const std::string& name) {
    auto it = m_loadedScripts.find(name);
    if (it == m_loadedScripts.end()) {
        spdlog::error("ScriptManager: Requested script '{}' is not loaded.", name);
        return nullptr;
    }
    // Return a pointer to the stored protected_function
    return &it->second;
}

void ScriptManager::UpdateScriptSystem(float dt) {
    sol::state& lua = GetLuaState();
    sol::protected_function ecs_foreach = lua["ECS"]["ForEach"];

    if (!ecs_foreach.valid()) {
        spdlog::warn("ECS.ForEach not found. Script system is inactive.");
        return;
    }

    // Define the query: entities must have a "script" component
    std::vector<std::string> components_to_query = { "script" };

    // The Lua callback function: runs the script defined in the component
    // Note: The script component must be exposed to Lua as "script"
    auto script_callback = [&](int entity_id) {
        // Retrieve the script component data for this entity
        // We use sol::optional for safety, assuming the component exists (as per query)
        sol::optional<enDjinn::ScriptComponent> script_comp = lua["ECS"]["Components"]["script"][entity_id];

        if (script_comp && !script_comp->name.empty()) {
            // Find the global Lua function that corresponds to the script name, e.g., "Entity_Update"
            sol::protected_function entity_script_func = lua[script_comp->name.c_str()];

            if (entity_script_func.valid()) {
                // Execute the script function, passing the entity ID and delta time
                sol::protected_function_result result = entity_script_func(entity_id, dt);

                if (!result.valid()) {
                    sol::error err = result;
                    spdlog::error("Entity Script Runtime Error ({}) for Entity {}: {}",
                        script_comp->name, entity_id, err.what());
                }
            }
            else {
                spdlog::warn("Script function '{}' not found for Entity {}.", script_comp->name, entity_id);
            }
        }
        };

    // Execute the ECS query
    // sol::as_table ensures the C++ vector is passed as a Lua table (array)
    ecs_foreach(sol::as_table(components_to_query), script_callback);
}