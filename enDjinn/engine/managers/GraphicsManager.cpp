#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>
#include <vector>
#include <string>
#include "GraphicsManager.h"
#include "ScriptManager.h"
#include "./utils/Types.h"
#include "spdlog/spdlog.h"
#include <iostream>
#include <functional>
#include <string_view>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <array>

struct GLFWwindow;

struct Uniforms {
    glm::mat4 projection;
};


// Utility function to convert single value to const pointer for WGPU descriptors
template< typename T > constexpr const T* to_ptr(const T& val) { return &val; }
template< typename T, std::size_t N > constexpr const T* to_ptr(const T(&& arr)[N]) { return arr; }

// Utility function to get the preferred format of a WGPUSurface for a given WGPUAdapter
WGPUTextureFormat wgpuSurfaceGetPreferredFormat(WGPUSurface surface, WGPUAdapter adapter) {
    WGPUSurfaceCapabilities capabilities{};
    wgpuSurfaceGetCapabilities(surface, adapter, &capabilities);
    const WGPUTextureFormat result = capabilities.formats[0];
    wgpuSurfaceCapabilitiesFreeMembers(capabilities);
    return result;
}

namespace enDjinn {
	// Background color components
    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;

    GraphicsManager::GraphicsManager() {}

    GraphicsManager::~GraphicsManager() {
    }

	// Startup method implementation
    void GraphicsManager::Startup(int width, int height, const std::string& title, bool fullscreen) {
		// 0. Initialize GLFW
        if (!glfwInit()) {
            spdlog::error("Failed to initialize GLFW.");
            return;
        }

		// Configure GLFW for WebGPU
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		// Create the window
        m_window = glfwCreateWindow(width, height, title.c_str(), fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);
        if (!m_window) {
            spdlog::error("Failed to create a window.");
            glfwTerminate();
            return;
        }

		// Show the window
        glfwShowWindow(m_window);

		// Set aspect ratio
        glfwSetWindowAspectRatio(m_window, width, height);

        // 1. Initialize WebGPU
        WGPUInstanceDescriptor instanceDesc{};
        m_instance = wgpuCreateInstance(to_ptr(instanceDesc));
        if (!m_instance) {
            spdlog::error("Failed to create WebGPU instance.");
            glfwTerminate();
            return;
        }

        // 2. Create the Surface
        m_surface = glfwCreateWindowWGPUSurface(m_instance, m_window);

        if (!m_surface) {
            spdlog::error("Failed to create WebGPU surface.");
            // Properly terminate the previous steps
            if (m_instance) wgpuInstanceRelease(m_instance);
            glfwTerminate();
            return; // Stop initialization
        }

        // 3. Request an Adapter
        wgpuInstanceRequestAdapter(
            m_instance,
            to_ptr(WGPURequestAdapterOptions{ .compatibleSurface = m_surface }),
            WGPURequestAdapterCallbackInfo{
                .mode = WGPUCallbackMode_AllowSpontaneous,
                .callback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void* adapter_ptr, void*) {
                    if (status != WGPURequestAdapterStatus_Success) {
                        std::cerr << "Failed to get a WebGPU adapter: " << std::string_view(message.data, message.length) << std::endl;
                        glfwTerminate();
                    }
                    *static_cast<WGPUAdapter*>(adapter_ptr) = adapter;
                },
                .userdata1 = &m_adapter
            }
        );
        while (!m_adapter) wgpuInstanceProcessEvents(m_instance);
        assert(m_adapter);

        // 4. Request a Device
        WGPUDeviceDescriptor deviceDesc{};
        deviceDesc.uncapturedErrorCallbackInfo.callback = [](WGPUDevice const*, WGPUErrorType type, WGPUStringView message, void*, void*) {
            std::cerr << "WebGPU uncaptured error type " << int(type) << " with message: " << std::string_view(message.data, message.length) << std::endl;
            };
        deviceDesc.uncapturedErrorCallbackInfo.userdata1 = nullptr;

        wgpuAdapterRequestDevice(
            m_adapter,
            to_ptr(deviceDesc),
            WGPURequestDeviceCallbackInfo{
                .mode = WGPUCallbackMode_AllowSpontaneous,
                .callback = [](WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void* device_ptr, void*) {
                    if (status != WGPURequestDeviceStatus_Success) {
                        std::cerr << "Failed to get a WebGPU device: " << std::string_view(message.data, message.length) << std::endl;
                        glfwTerminate();
                    }
                    *static_cast<WGPUDevice*>(device_ptr) = device;
                },
                .userdata1 = &m_device
            }
        );
        while (!m_device) wgpuInstanceProcessEvents(m_instance);
        assert(m_device);

        // 5. Get the Queue
        m_queue = wgpuDeviceGetQueue(m_device);

        m_uniformBuffer = wgpuDeviceCreateBuffer(m_device, to_ptr(WGPUBufferDescriptor{
            .label = WGPUStringView("Uniform Buffer", WGPU_STRLEN),
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
            .size = sizeof(Uniforms)
            }));

		// Calculate initial projection matrix
        int windowWidth, windowHeight;

		// Get the current framebuffer size
        glfwGetFramebufferSize(m_window, &windowWidth, &windowHeight);

		// Calculate the projection matrix
        Uniforms uniforms;
        uniforms.projection = glm::mat4(1.0f); // Start with identity

        // Scale x and y by 1/100 (World coordinates -100 to 100)
        uniforms.projection[0][0] = uniforms.projection[1][1] = 1.0f / 100.0f;

        // Apply aspect ratio correction (scaling the long edge down)
        if (windowWidth < windowHeight) {
            uniforms.projection[1][1] *= (float)windowWidth / (float)windowHeight;
        }
        else {
            uniforms.projection[0][0] *= (float)windowHeight / (float)windowWidth;
        }

		// Upload the projection matrix to the uniform buffer
        wgpuQueueWriteBuffer(m_queue, m_uniformBuffer, 0, &uniforms, sizeof(Uniforms));

		// Create the Sampler
        m_sampler = wgpuDeviceCreateSampler(m_device, to_ptr(WGPUSamplerDescriptor{
             .addressModeU = WGPUAddressMode_ClampToEdge,
             .addressModeV = WGPUAddressMode_ClampToEdge,
             .magFilter = WGPUFilterMode_Linear,
             .minFilter = WGPUFilterMode_Linear,
             .maxAnisotropy = 1
            }));
        assert(m_sampler);

        // 6. Define the Shaders
        const char* source = R"(
            struct Uniforms {
                projection: mat4x4f,
            };

            @group(0) @binding(0) var<uniform> uniforms: Uniforms;
            @group(0) @binding(1) var texSampler: sampler;
            @group(0) @binding(2) var texData: texture_2d<f32>;
    
            struct VertexInput {
                @location(0) position: vec2f,
                @location(1) texcoords: vec2f,
                @location(2) translation: vec3f,
                @location(3) scale: vec2f,
            };
    
            struct VertexOutput {
                @builtin(position) position: vec4f,
                @location(0) texcoords: vec2f,
            };
    
            @vertex fn vertex_shader_main(in: VertexInput) -> VertexOutput {
                var out: VertexOutput;
                out.position = uniforms.projection * vec4f(vec3f(in.scale * in.position, 0.0) + in.translation, 1.0);
                out.texcoords = in.texcoords;
                return out;
            }
    
            @fragment fn fragment_shader_main(in: VertexOutput) -> @location(0) vec4f {
                let color = textureSample(texData, texSampler, in.texcoords).rgba;
                return color;
            }
            )";

        // 7. Create the Shader Module
        WGPUShaderSourceWGSL source_desc = {};
        source_desc.chain.sType = WGPUSType_ShaderSourceWGSL;
        source_desc.code = WGPUStringView(source, std::string_view(source).length());
        // Point to the code descriptor from the shader descriptor.
        WGPUShaderModuleDescriptor shader_desc = {};
        shader_desc.nextInChain = &source_desc.chain;
        WGPUShaderModule shader_module = wgpuDeviceCreateShaderModule(m_device, &shader_desc);

        // 8. Create the Vertex Buffer
        const struct {
            float x, y;
            float u, v;
        } vertices[] = {
            { -1.0f, -1.0f, 0.0f, 1.0f },
            { 1.0f, -1.0f, 1.0f, 1.0f },
            { -1.0f, 1.0f, 0.0f, 0.0f },
            { 1.0f, 1.0f, 1.0f, 0.0f },
        };

		// Create the vertex buffer
        m_vertexBuffer = wgpuDeviceCreateBuffer(m_device, to_ptr(WGPUBufferDescriptor{
            .label = WGPUStringView("Vertex Buffer", WGPU_STRLEN),
            .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
            .size = sizeof(vertices)
            }));

		// Check for successful creation and upload vertex data
        if (!m_vertexBuffer) {
            spdlog::error("Failed to create vertex buffer!");
        }
        wgpuQueueWriteBuffer(m_queue, m_vertexBuffer, 0, vertices, sizeof(vertices));

		// 9. Create the Render Pipeline
        struct InstanceData {
            vec3 translation;
            vec2 scale;
            // rotation? Would add if I decide to support the engine
        };

		// Configure the surface
        glfwGetFramebufferSize(m_window, &width, &height);
        wgpuSurfaceConfigure(m_surface, to_ptr(WGPUSurfaceConfiguration{
            .device = m_device,
            .format = wgpuSurfaceGetPreferredFormat(m_surface, m_adapter),
            .usage = WGPUTextureUsage_RenderAttachment,
            .width = (uint32_t)width,
            .height = (uint32_t)height,
            .presentMode = WGPUPresentMode_Fifo // Explicitly set this because of a Dawn bug
            }));

		// Create the render pipeline
        m_renderPipeline = wgpuDeviceCreateRenderPipeline(m_device, to_ptr(WGPURenderPipelineDescriptor{
            // Describe the vertex shader inputs
            .vertex = {
                .module = shader_module,
                .entryPoint = WGPUStringView{ "vertex_shader_main", std::string_view("vertex_shader_main").length() },
                // Vertex attributes.
                .bufferCount = 2,
                .buffers = to_ptr<WGPUVertexBufferLayout>({
				// First buffer: static quad vertices
                {
                    .stepMode = WGPUVertexStepMode_Vertex,
                    .arrayStride = 4 * sizeof(float),
                    .attributeCount = 2,
                    .attributes = to_ptr<WGPUVertexAttribute>({
                        // Position x,y are first.
                        {
                            .format = WGPUVertexFormat_Float32x2,
                            .offset = 0,
                            .shaderLocation = 0
                        },
                        // Texture coordinates u,v are second.
                        {
                            .format = WGPUVertexFormat_Float32x2,
                            .offset = 2 * sizeof(float),
                            .shaderLocation = 1
                        }
                        })
                },
                    // We will use a second buffer with our per-sprite translation and scale. This data will be set in our draw function.
                    {
                        .stepMode = WGPUVertexStepMode_Instance,
                        .arrayStride = sizeof(InstanceData),
                        .attributeCount = 2,
                        .attributes = to_ptr<WGPUVertexAttribute>({
                        // Translation as a 3D vector.
                        {
                            .format = WGPUVertexFormat_Float32x3,
                            .offset = offsetof(InstanceData, translation),
                            .shaderLocation = 2
                        },
                            // Scale as a 2D vector for non-uniform scaling.
                            {
                                .format = WGPUVertexFormat_Float32x2,
                                .offset = offsetof(InstanceData, scale),
                                .shaderLocation = 3
                            }
                            })
                    }
                    })
                },

            // Interpret our 4 vertices as a triangle strip
            .primitive = WGPUPrimitiveState{
                .topology = WGPUPrimitiveTopology_TriangleStrip,
                },

                // No multi-sampling (1 sample per pixel, all bits on).
                .multisample = WGPUMultisampleState{
                    .count = 1,
                    .mask = ~0u
                    },

            // Describe the fragment shader and its output
            .fragment = to_ptr(WGPUFragmentState{
                .module = shader_module,
                .entryPoint = WGPUStringView{ "fragment_shader_main", std::string_view("fragment_shader_main").length() },

                // Our fragment shader outputs a single color value per pixel.
                .targetCount = 1,
                .targets = to_ptr<WGPUColorTargetState>({
                    {
						// The format must match the surface's preferred format.
                        .format = wgpuSurfaceGetPreferredFormat(m_surface, m_adapter),
						// The images we want to draw may have transparency, so alpha blending is needed.
                        // This will blend with whatever has already been drawn.
                        .blend = to_ptr(WGPUBlendState{
                        // Over blending for color
                        .color = {
                            .operation = WGPUBlendOperation_Add,
                            .srcFactor = WGPUBlendFactor_SrcAlpha,
                            .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha
                            },
                            // Leave destination alpha alone
                            .alpha = {
                                .operation = WGPUBlendOperation_Add,
                                .srcFactor = WGPUBlendFactor_Zero,
                                .dstFactor = WGPUBlendFactor_One
                                }
                            }),
                        .writeMask = WGPUColorWriteMask_All
                    }})
                })
            }));
        // Release the shader module
        wgpuShaderModuleRelease(shader_module);

		// Log successful startup messages
        spdlog::info("Window created successfully.");
        spdlog::info("WebGPU initialized and pipeline created.");
		spdlog::info("Graphics manager started up.");
    }

	//  Shutdown method implementation
    void GraphicsManager::Shutdown() {
        if (m_renderPipeline) wgpuRenderPipelineRelease(m_renderPipeline);
        if (m_sampler) wgpuSamplerRelease(m_sampler);
        if (m_uniformBuffer) wgpuBufferRelease(m_uniformBuffer);
        if (m_vertexBuffer) wgpuBufferRelease(m_vertexBuffer);
        if (m_queue) wgpuQueueRelease(m_queue);
        if (m_device) wgpuDeviceRelease(m_device);
        if (m_adapter) wgpuAdapterRelease(m_adapter);

        if (m_instance) wgpuInstanceRelease(m_instance);

        if (m_window) {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
        glfwTerminate();
        spdlog::info("Graphics manager shut down.");
    }

	// Draw method implementation
    void GraphicsManager::Draw() {
		// 1. Pre draw checks
        // We cannot draw if we don't have access to the script manager to query the ECS.
        if (!m_scriptManager) {
            spdlog::warn("GraphicsManager::Draw: ScriptManager is not set. Cannot render entities.");
            return;
        }

		// Get the Lua state from the ScriptManager
		// This is new following the ECS integration. We need to query entities from Lua.
        sol::state& lua = m_scriptManager->GetLuaState();
        sol::protected_function ecs_foreach = lua["ECS"]["ForEach"];

        if (!ecs_foreach.valid()) {
            spdlog::warn("GraphicsManager::Draw: ECS.ForEach not found in Lua. Cannot draw entities.");
            return;
        }

		// 2. ECS Querying
        // This is the core change: Instead of receiving a vector, we build one
        // by querying the ECS for all entities that have a "Sprite" component.
        std::vector<Sprite> sprites_from_ecs;
        std::vector<std::string> components_to_query = { "Sprite" };

        // This C++ lambda is called from Lua for each entity that matches the query.
        ecs_foreach(sol::as_table(components_to_query), [&](int entity_id) {
            // Safely retrieve the Sprite component from the Lua table.
            sol::optional<Sprite> sprite_comp = lua["ECS"]["Components"]["Sprite"][entity_id];
            if (sprite_comp) {
                sprites_from_ecs.push_back(*sprite_comp);
            }
            });

		// 3. Sorting Sprites by Z-Order
        // Sort the collected sprites from back-to-front based on their Z-value.
        // This ensures correct alpha blending for transparent images.
        std::sort(sprites_from_ecs.begin(), sprites_from_ecs.end(), [](const Sprite& lhs, const Sprite& rhs) {
            return lhs.z > rhs.z; // Higher Z is farther away, so it's drawn first.
            });

        size_t instanceCount = sprites_from_ecs.size();

		// 4. Render Pass Setup
        // Create an encoder to build the command buffer.
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, nullptr);

        // Get the texture view from the window's surface that we will draw into.
        WGPUSurfaceTexture surface_texture{};
        wgpuSurfaceGetCurrentTexture(m_surface, &surface_texture);
        WGPUTextureView current_texture_view = wgpuTextureCreateView(surface_texture.texture, nullptr);

        // Begin the render pass. This clears the screen to our background color.
        WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(encoder, to_ptr<WGPURenderPassDescriptor>({
            .colorAttachmentCount = 1,
            .colorAttachments = to_ptr<WGPURenderPassColorAttachment>({{
                .view = current_texture_view,
                .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
                .loadOp = WGPULoadOp_Clear,
                .storeOp = WGPUStoreOp_Store,
                .clearValue = WGPUColor{red, green, blue, 1.0} // Background color
            }})
            }));

        // If there are no sprites, we still need to clear the screen, but we can skip the drawing logic.
        if (instanceCount > 0) {
            // Create a GPU buffer large enough to hold the instance data for every sprite.
            WGPUBuffer instance_buffer = wgpuDeviceCreateBuffer(m_device, to_ptr<WGPUBufferDescriptor>({
                .label = WGPUStringView("Instance Buffer", WGPU_STRLEN),
                .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
                .size = sizeof(InstanceData) * instanceCount
                }));

            // Set the rendering pipeline that defines our shaders and vertex layouts.
            wgpuRenderPassEncoderSetPipeline(render_pass, m_renderPipeline);

            // Set the static vertex buffer (the quad) to shader location slot 0.
            wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, m_vertexBuffer, 0, 4 * 4 * sizeof(float));

            // Set the dynamic instance buffer (translations/scales) to shader location slot 1.
            wgpuRenderPassEncoderSetVertexBuffer(render_pass, 1, instance_buffer, 0, sizeof(InstanceData) * instanceCount);

			// 5. Main Draw Loop
            std::string currentTextureName = ""; // Track the last used texture to optimize bind group changes.
            WGPUBindGroup currentBindGroup = nullptr; // The currently active bind group.

            for (size_t i = 0; i < instanceCount; ++i) {
                const Sprite& sprite = sprites_from_ecs[i];

				// A. Update Instance Data
                InstanceData instance_data;
                instance_data.translation = glm::vec3(sprite.position, sprite.z);

                const Texture* loadedTexture = m_resourceManager->GetTexture(sprite.textureName);
                if (!loadedTexture || !loadedTexture->texture) {
                    spdlog::warn("Skipping sprite with missing texture: '{}'", sprite.textureName);
                    continue; // Skip this sprite if its texture isn't loaded.
                }

                // Correct the sprite's scale based on the image's aspect ratio.
                glm::vec2 aspect_scale(1.0f);
                if (loadedTexture->width < loadedTexture->height) {
                    aspect_scale.x = static_cast<float>(loadedTexture->width) / loadedTexture->height;
                }
                else {
                    aspect_scale.y = static_cast<float>(loadedTexture->height) / loadedTexture->width;
                }
                instance_data.scale = sprite.scale * aspect_scale;

                // Write this sprite's instance data to the GPU buffer at the correct offset.
                wgpuQueueWriteBuffer(m_queue, instance_buffer, i * sizeof(InstanceData), &instance_data, sizeof(InstanceData));

				// B. Update Bind Group if Texture Changed
                if (sprite.textureName != currentTextureName) {
                    if (currentBindGroup) {
                        wgpuBindGroupRelease(currentBindGroup); // Release the old one.
                    }

                    currentTextureName = sprite.textureName;
                    WGPUTexture tex = loadedTexture->texture;

                    WGPUBindGroupLayout layout = wgpuRenderPipelineGetBindGroupLayout(m_renderPipeline, 0);
                    WGPUTextureView textureView = wgpuTextureCreateView(tex, nullptr);

                    std::array<WGPUBindGroupEntry, 3> entries{};
                    entries[0] = { .binding = 0, .buffer = m_uniformBuffer, .size = sizeof(Uniforms) };
                    entries[1] = { .binding = 1, .sampler = m_sampler };
                    entries[2] = { .binding = 2, .textureView = textureView };

                    currentBindGroup = wgpuDeviceCreateBindGroup(m_device, to_ptr(WGPUBindGroupDescriptor{
                        .layout = layout,
                        .entryCount = entries.size(),
                        .entries = entries.data()
                        }));

                    // The bind group now holds a reference to the view, so we can release our handle to it.
                    wgpuTextureViewRelease(textureView);
                    wgpuBindGroupLayoutRelease(layout);

                    // Set the newly created bind group for subsequent draw calls.
                    wgpuRenderPassEncoderSetBindGroup(render_pass, 0, currentBindGroup, 0, nullptr);
                }

				// C. Issue the Draw Call
                // Draw 4 vertices (our quad) using 1 instance, starting at vertex 0 and instance `i`.
                wgpuRenderPassEncoderDraw(render_pass, 4, 1, 0, static_cast<uint32_t>(i));
            }

			// 6. Cleanup after drawing all sprites
            if (currentBindGroup) {
                wgpuBindGroupRelease(currentBindGroup);
            }
            wgpuBufferRelease(instance_buffer);
        }

		// 7. Finalize the Render Pass
        wgpuRenderPassEncoderEnd(render_pass);
        WGPUCommandBuffer command_buffer = wgpuCommandEncoderFinish(encoder, nullptr);
        wgpuQueueSubmit(m_queue, 1, &command_buffer);
        wgpuSurfacePresent(m_surface);

		// 8. Release temporary resources
        wgpuTextureViewRelease(current_texture_view);
        wgpuTextureRelease(surface_texture.texture);
        wgpuCommandEncoderRelease(encoder);
        wgpuCommandBufferRelease(command_buffer);
    }

	// ShouldClose method implementation. Needed to close window from input
    bool GraphicsManager::ShouldClose() const {
        if (!m_window) {
            spdlog::info("GraphicsManager::ShouldClose called before window was created.");
            return true;
        }
        return glfwWindowShouldClose(m_window);
    }

	// CalculateProjection method implementation. Helper method to calculate projection matrix
    void GraphicsManager::GetWindowDimensions(int& width, int& height) const {
        if (m_window) {
            // Use the GLFW function to get the current framebuffer size
            // which represents the drawable area in pixels.
            glfwGetFramebufferSize(m_window, &width, &height);
        }
        else {
            // Fallback or error state
            width = 0;
            height = 0;
            spdlog::warn("GraphicsManager::GetWindowDimensions called before window was created.");
        }
    }

	// Get GLFWwindow method implementation
    GLFWwindow* GraphicsManager::GetWindow() const {
        return m_window;
    }
	// Get WebGPU device method implementation
    WGPUDevice GraphicsManager::GetDevice() const { 
        return m_device; 
    }
	// Get WebGPU queue method implementation
    WGPUQueue GraphicsManager::GetQueue() const { 
        return m_queue;
    }

} // namespace enDjinn