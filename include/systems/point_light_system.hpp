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

class PointLightSystem {
    public:
        PointLightSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
        ~PointLightSystem();

        PointLightSystem(const PointLightSystem &) = delete;
        PointLightSystem &operator=(const PointLightSystem &) = delete;
    
        void update(FrameInfo &frameInfo, GlobalUbo &ubo);
        void render(FrameInfo &frameInfo);
    private:
        void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
        void createPipeline(VkRenderPass renderPass);

        Device &device;

        std::unique_ptr<Pipeline> pipeline;
        VkPipelineLayout pipelineLayout;
};

}
