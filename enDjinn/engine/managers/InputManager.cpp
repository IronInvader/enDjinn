#include "InputManager.h"
#include <iostream>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

namespace enDjinn {

    InputManager::InputManager(GLFWwindow* window) : m_window(window) {
    }

	// IsKeyPressed method implementation. Checks if a key is currently pressed and/or held down
    bool InputManager::IsKeyPressed(int key) const {
		if (!m_window) { //Safety check and logs
            spdlog::info("InputManager: Window is null, cannot check key.");
            return false;
        }

        // Check if the key state is either pressed or repeating
        int state = glfwGetKey(m_window, key);
        if (state == GLFW_PRESS || state == GLFW_REPEAT) {
            return true;
        }

        return false;
    }

} // namespace enDjinn