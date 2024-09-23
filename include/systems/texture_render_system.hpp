#pragma once

#include "../pipeline.hpp"
#include "../device.hpp"
#include "../game_object.hpp"
#include "../camera.hpp"
#include "../frame_info.hpp"

#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace vkengine {

class TextureRenderSystem {
    public:
        TextureRenderSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
        ~TextureRenderSystem();

        TextureRenderSystem(const TextureRenderSystem &) = delete;
        TextureRenderSystem &operator=(const TextureRenderSystem &) = delete;

        void renderGameObjects(FrameInfo &frameInfo);
    private:
        void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
        void createPipeline(VkRenderPass renderPass);

        Device &device;

        std::unique_ptr<Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;
};

}
