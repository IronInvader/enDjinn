#include "ResourceManager.h"
#include "./managers/GraphicsManager.h"
#include "spdlog/spdlog.h"
#include <cstddef> // For offsetof, if needed later

// --- STB_IMAGE Implementation ---
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" 
// --- END STB_IMAGE Implementation ---

// Utility function to convert single value to const pointer for WGPU descriptors
template< typename T > constexpr const T* to_ptr(const T& val) { return &val; }

namespace enDjinn {

    ResourceManager::ResourceManager(GraphicsManager* gm)
        : m_graphicsManager(gm), // Initialize the new member
        m_assetRoot("assets")
    {
        spdlog::info("ResourceManager initialized. Default asset root: {}", m_assetRoot.string());
    }

    ResourceManager::~ResourceManager() {
        // The map destructor will automatically call the Texture destructor for every element.
    }


    std::filesystem::path ResourceManager::ResolvePath(const std::string& partialPath) const {
        std::filesystem::path fullPath = m_assetRoot / partialPath;

        // Optional: Normalize the path to forward slashes for cross-platform consistency
        std::string normalizedPath = fullPath.generic_string();

        spdlog::debug("ResourceManager: Resolving '{}' to '{}'", partialPath, normalizedPath);
        return fullPath; // Return the path object, let stbi_load handle conversion
    }

    void ResourceManager::SetAssetRoot(const std::filesystem::path& newRoot) {
        m_assetRoot = newRoot;
        spdlog::info("ResourceManager: Asset root set to '{}'", m_assetRoot.string());
		// Note: This was needed due to the nature of Visual Studio. However, it may still be useful for flexibility.
    }

    // --- Texture Loading Logic ---

    bool ResourceManager::LoadTexture(const std::string& name, const std::string& partialPath) {
		// Validation of graphics context
        if (!m_graphicsManager || !m_graphicsManager->GetDevice() || !m_graphicsManager->GetQueue()) {
            spdlog::error("ResourceManager: Graphics context not initialized.");
            return false;
        }

		// Validation of unique name
        if (m_textures.count(name)) {
            spdlog::warn("ResourceManager: Texture with name '{}' already loaded.", name);
            return true;
        }

        // 1. Resolve path and load data from disk
        std::filesystem::path fullPath = ResolvePath(partialPath);
        int width, height, channels;

        std::string path_for_stbi = fullPath.generic_string();

        // Log the path being used for stbi_load
        spdlog::info("Attempting to load texture from generic path: '{}'", path_for_stbi);

        // Use the consistent, generic path for loading
        unsigned char* data = stbi_load(path_for_stbi.c_str(), &width, &height, &channels, 4);

        if (!data) {
            // Log the failure reason clearly
            spdlog::error("ResourceManager: Failed to load image from path '{}'. Reason: {}", path_for_stbi, stbi_failure_reason());
            return false;
        }

        // 2. Create WGPUTexture on GPU
        WGPUTextureDescriptor texDesc{};
        texDesc.label = WGPUStringView(path_for_stbi.c_str(), WGPU_STRLEN);
        texDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
        texDesc.dimension = WGPUTextureDimension_2D;
        texDesc.size = { (uint32_t)width, (uint32_t)height, 1 };
        texDesc.format = WGPUTextureFormat_RGBA8UnormSrgb;
        texDesc.mipLevelCount = 1;
        texDesc.sampleCount = 1;

        WGPUTextureFormat viewFormat = WGPUTextureFormat_RGBA8UnormSrgb;
        texDesc.viewFormatCount = 1;
        texDesc.viewFormats = &viewFormat;

        WGPUTexture tex = wgpuDeviceCreateTexture(m_graphicsManager->GetDevice(), &texDesc);
        if (!tex) {
            spdlog::error("ResourceManager: Failed to create WGPUTexture for '{}'.", name);
            stbi_image_free(data);
            return false;
        }

        // 3. Copy image data to the GPU
        WGPUExtent3D textureExtent = { (uint32_t)width, (uint32_t)height, 1 };
        uint32_t bytesPerRow = (uint32_t)(width * 4); // 4 bytes per pixel (RGBA)
        size_t data_size = (size_t)(width * height * 4);

        // Prepare copy targets as local variables so pointers are stable
        WGPUTexelCopyTextureInfo copyTextureInfo{};
        copyTextureInfo.texture = tex;
        copyTextureInfo.mipLevel = 0;
        copyTextureInfo.origin = WGPUOrigin3D{ 0, 0, 0 };

		// Buffer layout
        WGPUTexelCopyBufferLayout bufferLayout{};
        bufferLayout.offset = 0;
        bufferLayout.bytesPerRow = bytesPerRow;
        bufferLayout.rowsPerImage = (uint32_t)height;

		// Define the extent of the texture to copy
        WGPUExtent3D extent{};
        extent.width = (uint32_t)width;
        extent.height = (uint32_t)height;
        extent.depthOrArrayLayers = 1;

		// Perform the texture data upload
        wgpuQueueWriteTexture(
            m_graphicsManager->GetQueue(),
            &copyTextureInfo,
            data,
            data_size,
            &bufferLayout,
            &extent
        );

		// Create a texture view (optional, depending on usage)
        WGPUTextureViewDescriptor viewDesc{};
        viewDesc.format = texDesc.format; // match texture format
        viewDesc.dimension = WGPUTextureViewDimension_2D;
        viewDesc.baseMipLevel = 0;
        viewDesc.mipLevelCount = 1;
        viewDesc.baseArrayLayer = 0;
        viewDesc.arrayLayerCount = 1;
        viewDesc.aspect = WGPUTextureAspect_All;

        WGPUTextureView textureView = wgpuTextureCreateView(tex, &viewDesc);
        if (!textureView) {
            spdlog::error("ResourceManager: Failed to create texture view for '{}'", name);
            wgpuTextureRelease(tex); // cleanup if desired
            stbi_image_free(data);
            return false;
        }

        // 4. Free CPU memory
        stbi_image_free(data);

        // 5. Store the texture in the map
        m_textures.emplace(name, Texture(
            width,
            height,
            tex
        ));

        return true;
    }

	// --- Texture Retrieval Logic ---
    const Texture* ResourceManager::GetTexture(const std::string& name) const {
        auto it = m_textures.find(name);
        if (it == m_textures.end()) {
            spdlog::error("ResourceManager: Requested texture '{}' not found.", name);
            return nullptr;
        }
        return &it->second;
    }

} // namespace enDjinn