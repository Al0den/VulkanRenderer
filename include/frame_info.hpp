#pragma once 

#include "camera.hpp"
#include "game_object.hpp"

#include <vulkan/vulkan.h>

namespace vkengine {

struct FrameInfo {
    int frameIndex;
    float frameTime;
    VkCommandBuffer commandBuffer;
    Camera &camera;
    VkDescriptorSet globalDescriptorSet;
    GameObject::Map &gameObjects;
};

}
