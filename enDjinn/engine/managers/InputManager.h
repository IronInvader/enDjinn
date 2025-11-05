#pragma once

#include <GLFW/glfw3.h>

namespace enDjinn {

    class InputManager {


    public:
        InputManager(GLFWwindow* window);

        bool IsKeyPressed(int key) const;

    private:
        GLFWwindow* m_window;
    };

} // namespace enDjinn
