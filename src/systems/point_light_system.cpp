#include "../../include/systems/point_light_system.hpp"

#include <stdexcept>
#include <cstdlib>
#include <ctime>

#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FRCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

using namespace vkengine;

struct PointLightPushConstant {
    glm::vec4 position{};
    glm::vec4 color{};
    float radius;
};

PointLightSystem::PointLightSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : device{device} {
    createPipelineLayout(globalSetLayout);
    createPipeline(renderPass);

    std::srand(std::time(0));
}

PointLightSystem::~PointLightSystem() {
    vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
}

void PointLightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PointLightPushConstant);
    
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }
}

void PointLightSystem::createPipeline(VkRenderPass renderPass) {
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    Pipeline::defaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.attributeDescriptions.clear();
    pipelineConfig.bindingDescriptions.clear();
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;
    pipeline = std::make_unique<Pipeline>(device, "shaders/point_light.vert.spv", "shaders/point_light.frag.spv", pipelineConfig);
}

void PointLightSystem::update(FrameInfo &frameInfo, GlobalUbo &ubo) {
    int lightIndex = 0;
    auto rotateLight = glm::rotate(glm::mat4(1.f), frameInfo.frameTime, {0.f, -1.f, 0.f});
    for (auto &kv : frameInfo.gameObjects) {
        auto& obj = kv.second;
        if(obj.pointLight == nullptr) continue;

        assert(lightIndex < MAX_LIGHTS);

        obj.transform.translation = glm::vec3(rotateLight * glm::vec4(obj.transform.translation, 1.f));


        ubo.pointLights[lightIndex].position = glm::vec4(obj.transform.translation, 1.0f);
        ubo.pointLights[lightIndex].color = glm::vec4(obj.color, obj.pointLight->lightIntensity);

        lightIndex += 1;

    }
    ubo.numLights = lightIndex;
}

void PointLightSystem::render(FrameInfo &frameInfo) {
    pipeline->bind(frameInfo.commandBuffer);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);

    for (auto &kv : frameInfo.gameObjects) {
        auto &obj = kv.second;
        if(obj.pointLight == nullptr) continue;


        PointLightPushConstant push{};
        push.position = glm::vec4(obj.transform.translation, 1.f);
        push.color = glm::vec4(obj.color, obj.pointLight->lightIntensity);
        push.radius = obj.transform.scale.x;

        vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PointLightPushConstant), &push);
        vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
    }
}
