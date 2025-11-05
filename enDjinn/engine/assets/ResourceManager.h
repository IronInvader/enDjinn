#pragma once

#include <string>
#include <filesystem>
#include <unordered_map>
#include <webgpu/webgpu.h>

namespace enDjinn {

	//Forward declaration from namespace enDjinn
    class GraphicsManager;

    struct Texture {
        int width = 0;
        int height = 0;
        WGPUTexture texture = nullptr;

        Texture(int w, int h, WGPUTexture t)
            : width(w), height(h), texture(t) {
        }

		Texture() = delete; // Disable default constructor
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&&) = default;
        Texture& operator=(Texture&&) = default;

        // Destructor to ensure the resource is released
        ~Texture() {
            if (texture) wgpuTextureRelease(texture);
        }
    };

    class ResourceManager {
    public:
        ResourceManager(GraphicsManager* gm);
        ~ResourceManager();

        // Asset Loading Functions
        bool LoadTexture(const std::string& name, const std::string& partialPath);
  
        // Path Management
        std::filesystem::path ResolvePath(const std::string& partialPath) const;
        void SetAssetRoot(const std::filesystem::path& newRoot);
        const Texture* GetTexture(const std::string& name) const;


    private:
        GraphicsManager* m_graphicsManager;
        std::filesystem::path m_assetRoot;

        // Asset Storage
        std::unordered_map<std::string, Texture> m_textures;
    };

} // namespace enDjinn