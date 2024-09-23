#include "../include/app.hpp"
#include "../include/buffer.hpp"
#include "../include/camera.hpp"
#include "../include/keyboard_controller.hpp"
#include "../include/systems/simple_render_system.hpp"
#include "../include/systems/point_light_system.hpp"
#include "../include/systems/texture_render_system.hpp"
#include "../include/imgui.hpp"
#include "../include/textures.hpp"

#include <chrono>
#include <iostream>
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
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    Texture texure{device, "textures/grass.png"};
    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = texure.getSampler();
    imageInfo.imageView = texure.getImageView();
    imageInfo.imageLayout = texure.getImageLayout();

    std::vector<VkDescriptorSet> globalDescriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i=0; i< globalDescriptorSets.size(); i++) {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        DescriptorWriter(*globalSetLayout, *globalPool)
            .writeBuffer(0, &bufferInfo)
            .writeImage(1, &imageInfo)
            .build(globalDescriptorSets[i]);
    }
 
    SimpleRenderSystem simpleRenderSystem{device, renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};
    PointLightSystem pointLightSystem{device, renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};
    TextureRenderSystem textureRenderSystem{device, renderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};

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
        camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 1000.f);
        if (auto commandBuffer = renderer.beginFrame()) {
            imgui.newFrame();
            int frameIndex = renderer.getFrameIndex();
            FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, camera, globalDescriptorSets[frameIndex], gameObjects};

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
            textureRenderSystem.renderGameObjects(frameInfo);
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
    floor.transform.translation = {0.0, 0.5f, 0.0f};
    floor.transform.scale = {3.f, 1.f, 3.f};

    gameObjects.emplace(floor.getId(), std::move(floor));

     std::vector<glm::vec3> lightColors{
      {1.f, .1f, .1f},
      {.1f, .1f, 1.f},
      {.1f, 1.f, .1f},
      {1.f, 1.f, .1f},
      {.1f, 1.f, 1.f},
      {1.f, 1.f, 1.f}  //
    };

    for(int i=0; i< lightColors.size(); i++) {
        auto pointLight = GameObject::makePointLight(0.2f);
        pointLight.color = lightColors[i];
        auto rotateLight = glm::rotate(glm::mat4(1.f), (i * glm::two_pi<float>()) / lightColors.size(), {0.f, -1.f, 0.f});
        pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
        gameObjects.emplace(pointLight.getId(), std::move(pointLight));
    }

    //Create a cube using 6 Quads
    std::shared_ptr<Model> quadModel = Model::createModelFromFile(device, "models/quad.obj");
    std::vector<GameObject> faces;
    faces.reserve(6);

    std::vector<std::vector<float>> translations = {
        {0.0f,  0.0f,  -0.5f},  // Front face (positive Z)
        {0.0f,  0.0f,  0.5f},  // Back face (positive Z)
        {-0.5f,  0.0f,  0.0f},  // Left face (positive Z)
        {0.5f,  0.0f,  0.0f},  // Right face (positive Z)
        {0.0f,  0.5f,  0.0f},  // Bottom face (positive Z)
        {0.0f,  -0.5f,  0.0f},  // Top face (positive Z)
    };

    std::vector<std::vector<float>> rotations = {
        {glm::radians(90.f), 0.0f, 0.0f},                 // Front face, no rotation
        {glm::radians(90.f), 0.0f, 0.0f},                 // Front face, no rotation
        {0, 0.0f, glm::radians(270.0f)},                 // Front face, no rotation
        {0, 0.0f, glm::radians(90.0f)},                 // Front face, no rotation
        {0, 0.0f, glm::radians(180.0f)},                 // Front face, no rotation
        {0, 0.0f, 0.0f},                 // Front face, no rotation
    };
    
    glm::vec3 globalTranslation = glm::vec3(0.0, 0.5f, 2.0);

    for (int i = 0; i < 6; i++) {
        auto gameObj = GameObject::createGameObject();
        faces.push_back(std::move(gameObj));
        faces[i].model = quadModel;
        faces[i].transform.translation = glm::vec3(translations[i][0], translations[i][1], translations[i][2]) + globalTranslation;
        faces[i].transform.rotation = glm::vec3(rotations[i][0], rotations[i][1], rotations[i][2]);
        faces[i].texture = std::make_unique<Texture>(device, "textures/grass.png");

        gameObjects.emplace(faces[i].getId(), std::move(faces[i]));
    }
}
