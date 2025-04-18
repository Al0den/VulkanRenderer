#include "../include/app.hpp"
#include "../include/buffer.hpp"
#include "../include/camera.hpp"
#include "../include/keyboard_controller.hpp"
#include "../include/systems/simple_render_system.hpp"
#include "../include/systems/point_light_system.hpp"
#include "../include/imgui.hpp"
#include "../include/chunk.hpp"
#include "../include/chunk_manager.hpp"

#include <chrono>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FRCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

using namespace vkengine;

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
    PointLightSystem pointLightSystem{device, renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};

    Camera camera{};
    camera.setViewDirection(glm::vec3(0.f), glm::vec3(0.0, 0.2f, 1.0f));

    auto viewerObject = GameObject::createGameObject(); 
    viewerObject->transform.translation.z -= 2.5f;

    KeyboardController cameraController{};

    auto currentTime = std::chrono::high_resolution_clock::now();

    while (!window.shouldClose()) {
        glfwPollEvents();

        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        cameraController.moveInPlaneXZ(window.getWindow(), viewerObject, frameTime);
        camera.setViewYXZ(viewerObject->transform.translation, viewerObject->transform.rotation);

        // Update chunks based on player position and view distance
        chunkManager->update(viewerObject->transform.translation, CHUNK_VIEW_DISTANCE, gameObjects);

        float aspect = renderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 1000.f);
        if (auto commandBuffer = renderer.beginFrame()) {
            imgui.newFrame();
            int frameIndex = renderer.getFrameIndex();
            FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, camera, globalDescriptorSets[frameIndex], gameObjects, textureManager};

            //update
            GlobalUbo ubo{};
            ubo.projection = camera.getProjection() ;
            ubo.view = camera.getView();
            pointLightSystem.update(frameInfo, ubo);
            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            uboBuffers[frameIndex]->flush();

            //render
            renderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(frameInfo);
            pointLightSystem.render(frameInfo);
            imgui.debugWindow(frameInfo);
            imgui.render(commandBuffer);
            renderer.endSwapChainRenderPass(commandBuffer); 
            renderer.endFrame();
        }
    }

    vkDeviceWaitIdle(device.device());
}

void App::loadGameObjects() {
    // Create a single point light
    auto pointLight = GameObject::makePointLight(0.5f);
    pointLight->color = {1.f, 1.f, 1.f};
    pointLight->transform.translation = {0.f, CHUNK_SIZE, 0.f};
    gameObjects.emplace(pointLight->getId(), pointLight);

    // Initial chunks will be created by the ChunkManager during the first update
    // We don't need to manually create chunks here anymore
}