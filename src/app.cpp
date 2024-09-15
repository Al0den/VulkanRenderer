#include "../include/app.hpp"

#include <stdexcept>
#include <vulkan/vulkan_core.h>

#include "../include/simple_render_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FRCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

using namespace vkengine;

App::App() {
    loadGameObjects();
}

App::~App() {}

void App::run() {
    SimpleRenderSystem simpleRenderSystem{device, renderer.getSwapChainRenderPass()};

    while (!window.shouldClose()) {
        glfwPollEvents();
        if(auto commandBuffer = renderer.beginFrame()) {
            renderer.beginSwapChainRenderPass(commandBuffer);
            simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects);
            renderer.endSwapChainRenderPass(commandBuffer);
            renderer.endFrame();
        }
    }

    vkDeviceWaitIdle(device.device());
}

void App::loadGameObjects() {
    std::vector<Model::Vertex> vertices {
        {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    };

    auto model = std::make_shared<Model>(device, vertices);

    auto triangle = GameObject::createGameObject();
    triangle.model = model;
    triangle.color = {0.0f, 0.8f, 0.0f};
    triangle.transform2d.translation.x = 0.0f;
    triangle.transform2d.scale = {1.f, 1.0f};
    triangle.transform2d.rotation = 0.0f;

    gameObjects.push_back(std::move(triangle));

}
