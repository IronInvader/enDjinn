#include <glm/glm.hpp>
#include <string>

namespace enDjinn {

    // Forward declaration if needed, or put inside GraphicsManager/Engine namespace
    struct Sprite {
        std::string textureName; // Name used to look up in ResourceManager
        glm::vec2 position = { 0.0f, 0.0f }; // Translation (x, y)
        glm::vec2 scale = { 1.0f, 1.0f };     // Scale factor
        float z = 0.0f;                    // Z-depth for sorting (0.0=front, 1.0=back)
    };

}