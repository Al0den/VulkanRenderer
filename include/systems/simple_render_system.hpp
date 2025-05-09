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

        std::unique_ptr<Pipeline> texturedPipeline;
        std::unique_ptr<Pipeline> wireframePipeline;
        VkPipelineLayout pipelineLayout;
};

}
