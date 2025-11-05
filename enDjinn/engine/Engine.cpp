#include "Engine.h"
#include "spdlog/spdlog.h"
#include "sokol_time.h"
#include <iostream>
#include "GLFW/glfw3.h"
#include <chrono>
#include <thread>

namespace enDjinn {
	// Constructor. Sets up unique pointers for various managers as needed
    Engine::Engine()
        : m_graphicsManager(std::make_unique<GraphicsManager>())
        , m_inputManager(nullptr)
        , m_resourceManager(new ResourceManager(m_graphicsManager.get()))
        , m_soundManager(nullptr)
        , m_scriptManager(nullptr)
    {
    }
	// Destructor
    Engine::~Engine() = default;

	// Startup method implementation
    void Engine::Startup() {
		// Initialize GraphicsManager and set window size/title
        m_graphicsManager->Startup(1280, 720, "enDjinn", false);

		// Initialize ResourceManager, SoundManager, InputManager, and ScriptManager
        GLFWwindow* window = m_graphicsManager->GetWindow();
        if (m_resourceManager) {
            m_resourceManager->SetAssetRoot("../../../engine/assets");
            m_soundManager = std::make_unique<SoundManager>(*m_resourceManager);
            m_soundManager->Startup();
            m_graphicsManager->SetResourceManager(m_resourceManager.get());
        }
        else {
            spdlog::error("ResourceManager not initialized, SoundManager will be unusable.");
        }

		// Initialize InputManager and ScriptManager if window is valid
        if (window) {
			// Initialize InputManager with the GLFW window
            m_inputManager = std::make_unique<InputManager>(window);

			// Initialize ScriptManager and expose other managers to Lua
            m_scriptManager = std::make_unique<ScriptManager>();
            m_scriptManager->Startup();

			// Link ScriptManager to GraphicsManager before exposure (Can be moved)
			m_graphicsManager->SetScriptManager(m_scriptManager.get());

            // --- BINDING FIX ---
            m_scriptManager->ExposeInputManager(m_inputManager.get());
            m_scriptManager->ExposeResourceManager(m_resourceManager.get());
            m_scriptManager->ExposeSoundManager(m_soundManager.get());

			// Load Scripts: ECS first, then main_script
            bool scriptLoaded = m_scriptManager->LoadScript(
                "main_script",
                m_resourceManager->ResolvePath("scripts/main_script.lua").generic_string()
            );

            bool ECSLoaded = m_scriptManager->LoadScript(
                "ECS",
                m_resourceManager->ResolvePath("scripts/ecs.lua").generic_string()
            );

			// Other scripts can be loaded here as needed

			// Execute the loaded scripts
            sol::protected_function* setup_chunk = m_scriptManager->GetScript("main_script");
            sol::protected_function* ecs_chunk = m_scriptManager->GetScript("ECS");

			// Execute ECS script first to define ECS table, then main script
			// THIS ORDER IS CRUCIAL. Main script depends on ECS being defined.
            if (setup_chunk && ecs_chunk) {
                // 1. EXECUTE THE ECS SCRIPT FIRST
                sol::protected_function_result ecs_result = (*ecs_chunk)();

				// Check for errors during ECS script execution
                if (!ecs_result.valid()) {
                    sol::error err = ecs_result;
                    spdlog::error("Lua Runtime Error during ECS script execution: {}", err.what());
                    // Handle critical failure
                    return;
                }
                else {
                    spdlog::info("ECS script executed successfully, ECS table is now defined.");
                }

                // 2. NOW EXECUTE THE MAIN SCRIPT
                sol::protected_function_result result = (*setup_chunk)();

				// Check for errors during main script execution
                if (!result.valid()) {
                    sol::error err = result;
                    spdlog::error("Lua Runtime Error during SETUP script execution: {}", err.what());
                    // Handle critical failure
                    return;
                }
                else {
                    spdlog::info("Successfully executed setup script once.");
                }
            }
			// --- END SCRIPT LOADING AND EXECUTION ---
            sol::state& lua = m_scriptManager->GetLuaState();

			// Bind the QuitGame function to Lua
            lua.set_function("QuitGame", [this]() {
                this->QuitGame();
                });
        }
        else {
            spdlog::error("GraphicsManager failed to create a window, InputManager will be unusable.");

        }

        // Startup SoundManager if ResourceManager is started up
        stm_setup();
        m_lastTime = stm_now(); // Initialize the starting time
        spdlog::info("Engine started up.");
    }

	//  Shutdown method implementation
    void Engine::Shutdown() {
        if (m_soundManager) {
            m_soundManager->Shutdown();
        }
        m_graphicsManager->Shutdown();
        spdlog::info("Engine shut down.");
    }

	// Responsive game loop implementation
    void Engine::RunGameLoop(const UpdateCallback& update_callback) {
        // We will run our game logic at a fixed 60 ticks per second.
        const double SECONDS_PER_TICK = 1.0 / 60.0;

        // An "accumulator" to track how much real time has passed that we haven't simulated yet.
        double accumulated_time_s = 0.0;

        // Get the starting time using std::chrono::steady_clock.
        // steady_clock is crucial because it's not affected by system time changes (e.g., daylight saving).
        auto last_time = std::chrono::steady_clock::now();

        spdlog::info("Entering responsive game loop (using std::chrono).");
        while (!m_graphicsManager->ShouldClose()) {
            // 1. Calculate Delta Time
            auto current_time = std::chrono::steady_clock::now();
            // The duration is a special type; .count() gives us the value in seconds (because we specified <double>).
            double delta_time_s = std::chrono::duration<double>(current_time - last_time).count();
            last_time = current_time;

            // Add the real time that passed to our accumulator.
            accumulated_time_s += delta_time_s;

            // 2. Poll for OS Events
            glfwPollEvents();

			// 3. Fixed frame rate update loop
            // This loop ensures your game logic runs at a consistent rate.
            while (accumulated_time_s >= SECONDS_PER_TICK) {
                update_callback();
                accumulated_time_s -= SECONDS_PER_TICK;
            }
        }
		// Exit message when loop ends
        spdlog::info("Game loop terminated.");
    }

	// Getter methods for various managers
    GraphicsManager* Engine::GetGraphicsManager() const {
        return m_graphicsManager.get();
    }

    InputManager* Engine::GetInputManager() const {
        return m_inputManager.get();
    }

    ResourceManager* Engine::GetResourceManager() const {
        return m_resourceManager.get();
    }

    SoundManager* Engine::GetSoundManager() const {
        return m_soundManager.get();
    }

    ScriptManager* Engine::GetScriptManager() const {
        return m_scriptManager.get();
    }

	// QuitGame method implementation
    void Engine::QuitGame() {
        GLFWwindow* window = m_graphicsManager->GetWindow();
        if (window) {
            spdlog::info("QuitGame() called from Lua. Setting window close flag.");
            // This is the CRITICAL line: it tells GLFW the window should close.
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        else {
            spdlog::error("QuitGame failed: GLFW window is NULL.");
        }
    }
} // namespace enDjinn