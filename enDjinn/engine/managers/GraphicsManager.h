#pragma once

#include <string>
#include "./assets/Sprite.h"
#include <webgpu/webgpu.h>
#include "./assets/ResourceManager.h"

struct InstanceData {
    // Location 2 in WGSL: translation: vec3f
    glm::vec3 translation;
    // Location 3 in WGSL: scale: vec2f
    glm::vec2 scale;
};

struct GLFWwindow;

namespace enDjinn {
	// Forward declarations
    class ScriptManager;

    class GraphicsManager {
    public:
        GraphicsManager();
        ~GraphicsManager();

        void Startup(int width, int height, const std::string& title, bool fullscreen);
        void Shutdown();

        void Draw();

        void SetResourceManager(ResourceManager* rm) { m_resourceManager = rm; }
        void SetScriptManager(ScriptManager* sm) { m_scriptManager = sm; }
        bool ShouldClose() const;
        void CalculateProjection(glm::mat4& projection, unsigned int width, unsigned int height);
        GLFWwindow* GetWindow() const;

        WGPUDevice GetDevice() const;
        WGPUQueue GetQueue() const;

    private:
        void GetWindowDimensions(int& width, int& height) const;
        ResourceManager* m_resourceManager = nullptr;
		ScriptManager* m_scriptManager = nullptr;
        GLFWwindow* m_window = nullptr;

        // WebGPU objects
        WGPUInstance m_instance = nullptr;
        WGPUSurface m_surface = nullptr;
        WGPUAdapter m_adapter = nullptr;
        WGPUDevice m_device = nullptr;
        WGPUQueue m_queue = nullptr;

        // Drawing resources
        WGPUBuffer m_vertexBuffer = nullptr;
        WGPUBuffer m_uniformBuffer = nullptr;
        WGPUSampler m_sampler = nullptr;
        WGPURenderPipeline m_renderPipeline = nullptr;
    };

} // namespace enDjinn