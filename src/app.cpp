#include "../include/app.hpp"
#include "../include/buffer.hpp"
#include "../include/camera.hpp"
#include "../include/keyboard_controller.hpp"
#include "../include/systems/simple_render_system.hpp"
#include "../include/systems/point_light_system.hpp"

#include <chrono>
#include <vulkan/vulkan_core.h>


#define GLM_FORCE_RADIANS
#define GLM_FRCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

using namespace vkengine;

struct GlobalUbo {
    glm::mat4 projection{1.f};
    glm::mat4 view{1.f};
    glm::vec4 ambientLightColor{1.f, 1.f, 1.f, 0.02f};
    glm::vec3 lightPosition{-1.f};
    alignas(16) glm::vec4 lightColor{1.f};
};

App::App() { 
    globalPool = DescriptorPool::Builder(device)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
        .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
        .build();
    loadGameObjects(); 
} 

App::~App() {}

void App::run() {
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
    viewerObject.transform.translation.z -= 2.5f;

    KeyboardController cameraController{};

    auto currentTime = std::chrono::high_resolution_clock::now();

    while (!window.shouldClose()) {
        glfwPollEvents();

        auto newTime = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime = newTime;

        cameraController.moveInPlaneXZ(window.getWindow(), viewerObject, frameTime);
        camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

        float aspect = renderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 10.f);
        if (auto commandBuffer = renderer.beginFrame()) {
            int frameIndex = renderer.getFrameIndex();
            FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, camera, globalDescriptorSets[frameIndex], gameObjects};

            //update
            GlobalUbo ubo{};
            ubo.projection = camera.getProjection() ;
            ubo.view = camera.getView();
            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            uboBuffers[frameIndex]->flush();

            //render
            renderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(frameInfo);
            pointLightSystem.render(frameInfo);
            renderer.endSwapChainRenderPass(commandBuffer); 
            renderer.endFrame();
        }
    }

    vkDeviceWaitIdle(device.device());
}

void App::loadGameObjects() {
    std::shared_ptr<Model> model = Model::createModelFromFile(device, "models/smooth_vase.obj");
    auto gameObject = GameObject::createGameObject();
    gameObject.model = model;
    gameObject.transform.translation = {-0.5f, 0.5f, 0.0f};
    gameObject.transform.scale = {2.f, 1.f, 2.f};

    gameObjects.emplace(gameObject.getId(), std::move(gameObject));

    std::shared_ptr<Model> model2 = Model::createModelFromFile(device, "models/flat_vase.obj");
    auto gameObject2 = GameObject::createGameObject();
    gameObject2.model = model2;
    gameObject2.transform.translation = {0.5, 0.5f, 0.0f};
    gameObject2.transform.scale = {2.f, 1.f, 2.f};

    gameObjects.emplace(gameObject2.getId(), std::move(gameObject2));

    std::shared_ptr<Model> quad = Model::createModelFromFile(device, "models/quad.obj");
    auto floor = GameObject::createGameObject();
    floor.model = quad;
    floor.transform.translation = {0.5, 0.5f, 0.0f};
    floor.transform.scale = {3.f, 1.f, 3.f};

    gameObjects.emplace(floor.getId(), std::move(floor));
}
