#pragma once

#include "../pipeline.hpp"
#include "../device.hpp"
#include "../game_object.hpp"
#include "../camera.hpp"
#include "../frame_info.hpp"
#include "../descriptors.hpp"     // Added for DescriptorSetLayout
#include "../model.hpp"
#include "../texture_manager.hpp" // Added for TextureManager

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace vkengine {

class SimpleRenderSystem {
public:
    SimpleRenderSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
    ~SimpleRenderSystem();

    SimpleRenderSystem(const SimpleRenderSystem &) = delete;
    SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;

    void renderGameObjects(FrameInfo &frameInfo);

private:
    void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createPipelines(VkRenderPass renderPass);

    Device &device;

    std::unique_ptr<Pipeline> uvPipeline;
    std::unique_ptr<Pipeline> wireframePipeline;
    std::unique_ptr<Pipeline> texturePipeline;
    std::unique_ptr<Pipeline> colorPipeline;
    
    VkPipelineLayout pipelineLayout;

    std::unique_ptr<DescriptorSetLayout> textureSetLayout; // For texture atlas
    VkDescriptorSet textureDescriptorSet = VK_NULL_HANDLE; // For storing the texture atlas descriptor set
};

} // namespace vkengine
