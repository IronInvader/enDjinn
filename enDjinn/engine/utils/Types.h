#include "glm/glm.hpp"
#include <string>

#pragma once

typedef glm::vec2 vec2;
typedef glm::vec3 vec3;
typedef glm::vec4 vec4;

namespace enDjinn {
    enum KeyCode : int {
        // Alphanumeric keys
        KEY_SPACE = GLFW_KEY_SPACE,
        KEY_W = GLFW_KEY_W,
        KEY_A = GLFW_KEY_A,
        KEY_S = GLFW_KEY_S,
        KEY_D = GLFW_KEY_D,

        // Control keys
        KEY_ESCAPE = GLFW_KEY_ESCAPE,
        KEY_ENTER = GLFW_KEY_ENTER,
        KEY_LEFT_SHIFT = GLFW_KEY_LEFT_SHIFT,
		// ... (Include additional key codes as needed)
    };
	// ScriptComponent definition
	// Must be string, cannot be sol::string_view or function
    struct ScriptComponent {
        std::string name;
    };
}

