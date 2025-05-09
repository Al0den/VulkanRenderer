#include "../include/app.hpp"
#include "../include/buffer.hpp"
#include "../include/camera.hpp"
#include "../include/keyboard_controller.hpp"
#include "../include/systems/simple_render_system.hpp"
#include "../include/imgui.hpp"
#include "../include/chunk.hpp"
#include "../include/config.hpp"
#include "../include/chunk_manager.hpp"
#include "../include/scope_timer.hpp"

#include <chrono>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FRCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

using namespace vkengine;
using ScopeTimer = GlobalTimerData::ScopeTimer;

App::App() { 
    globalPool = DescriptorPool::Builder(device)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, SwapChain::MAX_FRAMES_IN_FLIGHT)
        .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
        .build();
    
    // Initialize chunk manager
    chunkManager = std::make_unique<ChunkManager>(device);
    
    loadGameObjects(); 
} 

App::~App() {}

void App::run() {
    Imgui imgui{window, device, renderer.getSwapChainRenderPass(), renderer.getImageCount()};

    std::vector<std::unique_ptr<Buffer>> uboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        uboBuffers[i] = std::make_unique<Buffer>(
            device,
            sizeof(GlobalUbo),
            1,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        uboBuffers[i]->map();
    }

    auto globalSetLayout = DescriptorSetLayout::Builder(device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
        .build();

    std::vector<VkDescriptorSet> globalDescriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i=0; i< globalDescriptorSets.size(); i++) {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        DescriptorWriter(*globalSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .build(globalDescriptorSets[i]);
    }
 
    SimpleRenderSystem simpleRenderSystem{device, renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};
    Camera camera{};
    camera.setViewDirection(glm::vec3(0.f), glm::vec3(0.0, 0.2f, 1.0f));

    auto viewerObject = GameObject::createGameObject(); 
    viewerObject->transform.translation.z -= 2.5f;

    KeyboardController cameraController{};

    auto currentTime = std::chrono::high_resolution_clock::now();

    // Debounce for render mode toggle
    bool rKeyPressedLastFrame = false;

    

    while (!window.shouldClose()) {
        ScopeTimer globalTimer("global");
        
        glfwPollEvents();

        // Render Mode Toggle Logic
        bool rKeyPressedThisFrame = glfwGetKey(window.getWindow(), GLFW_KEY_R) == GLFW_PRESS;
        if (rKeyPressedThisFrame && !rKeyPressedLastFrame) {
            if (static_cast<RenderMode>(config().getInt("render_mode")) == RenderMode::NORMAL_TEXTURED) {
                config().setInt("render_mode", static_cast<int>(RenderMode::WIREFRAME));
            } else if (static_cast<RenderMode>(config().getInt("render_mode")) == RenderMode::WIREFRAME) {
                // Cycle to the next mode or back to normal
                // For now, just toggles back to normal. 
                // Add more modes here in the future.
                config().setInt("render_mode", static_cast<int>(RenderMode::NORMAL_TEXTURED)); 
            }
        }
        rKeyPressedLastFrame = rKeyPressedThisFrame;

        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        cameraController.moveInPlaneXZ(window.getWindow(), viewerObject, frameTime);
        camera.setViewYXZ(viewerObject->transform.translation, viewerObject->transform.rotation);

        // Update chunks based on player position and view distance
        
        float aspect = renderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(config().getFloat("fov")), aspect, 0.1f, 1000.f);
        if (auto commandBuffer = renderer.beginFrame()) {
            
            {
                ScopeTimer timer("ChunkManager");
                
                chunkManager->update(viewerObject->transform.translation, config().getInt("render_distance"), gameObjects);
            }
            
            imgui.newFrame();
            int frameIndex = renderer.getFrameIndex();
            FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, camera, globalDescriptorSets[frameIndex], gameObjects, chunkManager.get()};

            //update
            GlobalUbo ubo{};
            ubo.projection = camera.getProjection() ;
            ubo.view = camera.getView();
            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            uboBuffers[frameIndex]->flush();

            //render
            renderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(frameInfo);
            imgui.debugWindow(frameInfo);
            imgui.render(commandBuffer);
            renderer.endSwapChainRenderPass(commandBuffer); 
            renderer.endFrame();
        }
    }

    vkDeviceWaitIdle(device.device());
}

void App::loadGameObjects() {}