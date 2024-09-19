#include "../include/app.hpp"
#include "../include/buffer.hpp"
#include "../include/camera.hpp"
#include "../include/keyboard_controller.hpp"
#include "../include/simple_render_system.hpp"

#include <chrono>
#include <vulkan/vulkan_core.h>


#define GLM_FORCE_RADIANS
#define GLM_FRCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

using namespace vkengine;

struct GlobalUbo {
    glm::mat4 projectionView{1.f};
    glm::vec3 lightDirection = glm::normalize(glm::vec3(1.f, -3.f, -1.f));
};

App::App() { loadGameObjects(); }

App::~App() {}

void App::run() {
    Buffer globalUboBuffer{
        device,
        sizeof(GlobalUbo),
        vkengine::SwapChain::MAX_FRAMES_IN_FLIGHT,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        device.properties.limits.minUniformBufferOffsetAlignment,
    };

    globalUboBuffer.map();

    SimpleRenderSystem simpleRenderSystem{device, renderer.getSwapChainRenderPass()};
    Camera camera{};
    camera.setViewDirection(glm::vec3(0.f), glm::vec3(0.0, 0.2f, 1.0f));

    auto viewerObject = GameObject::createGameObject();
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
            FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, camera};

            //update
            GlobalUbo ubo{};
            ubo.projectionView = camera.getProjection() * camera.getView();
            globalUboBuffer.writeToIndex(&ubo, frameIndex);
            globalUboBuffer.flushIndex(frameIndex);

            //render
            renderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(frameInfo, gameObjects);
            renderer.endSwapChainRenderPass(commandBuffer);
            renderer.endFrame();
        }
    }

    vkDeviceWaitIdle(device.device());
}

void App::loadGameObjects() {
    std::shared_ptr<Model> model = Model::createModelFromFile(device, "models/flat_vase.obj");
    auto gameObject = GameObject::createGameObject();
    gameObject.model = model;
    gameObject.transform.translation = {-0.7f, 0.5f, 2.5f};
    gameObject.transform.scale = {3.f, 1.f, 3.f};

    gameObjects.push_back(std::move(gameObject));

    std::shared_ptr<Model> model2 = Model::createModelFromFile(device, "models/flat_vase.obj");
    auto gameObject2 = GameObject::createGameObject();
    gameObject2.model = model;
    gameObject2.transform.translation = {0.7, 0.5f, 2.5f};
    gameObject2.transform.scale = {3.f, 1.f, 3.f};

    gameObjects.push_back(std::move(gameObject2));
}
