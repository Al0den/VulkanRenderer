#include "../include/app.hpp"
#include "../include/camera.hpp"
#include "../include/keyboard_controller.hpp"

#include <chrono>
#include <vulkan/vulkan_core.h>

#include "../include/simple_render_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FRCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

using namespace vkengine;

App::App() { loadGameObjects(); }

App::~App() {}

void App::run() {
    SimpleRenderSystem simpleRenderSystem{device, renderer.getSwapChainRenderPass()};
    Camera camera{};
    camera.setViewDirection(glm::vec3(0.f), glm::vec3(0.0, 0.2f, 1.0f));
    // camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.0, 0.0, 2.5));

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
        // camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
        camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 10.f);
        if (auto commandBuffer = renderer.beginFrame()) {
            renderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera);
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
    gameObject.transform.translation = {0.f, 0.5f, 2.5f};
    gameObject.transform.scale = glm::vec3(3.f);

    gameObjects.push_back(std::move(gameObject));
}
