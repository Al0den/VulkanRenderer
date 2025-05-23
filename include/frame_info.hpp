#pragma once 

#include "camera.hpp"
#include "game_object.hpp"
#include "chunk_manager.hpp"
#include "texture_manager.hpp"
#include "descriptors.hpp"

#include <vulkan/vulkan.h>

namespace vkengine {

#define MAX_LIGHTS 10

struct PointLight {
    glm::vec4 position{};
    glm::vec4 color{};
};

struct GlobalUbo {
    glm::mat4 projection{1.f};
    glm::mat4 view{1.f};
    glm::vec4 ambientLightColor{1.f, 1.f, 1.f, 0.02f};
};

struct FrameInfo {
    int frameIndex;
    float frameTime;
    VkCommandBuffer commandBuffer;
    Camera &camera;
    VkDescriptorSet globalDescriptorSet;
    GameObject::Map &gameObjects;
    ChunkManager* chunkManager = nullptr; // Pointer to the chunk manager. DO NOT REMOVE 
    std::shared_ptr<TextureManager> textureManager;
    std::shared_ptr<DescriptorPool> globalPool;
};

}
