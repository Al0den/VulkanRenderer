#include "../../include/systems/simple_render_system.hpp"
#include "../../include/config.hpp" // Added for g_currentRenderMode

#include <stdexcept>
#include <cstdlib>
#include <ctime>

#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FRCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

using namespace vkengine;

struct SimplePushConstantData {
    glm::mat4 modelMatrix{1.f};
    glm::mat4 normalMatrix{1.f};
};

SimpleRenderSystem::SimpleRenderSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : device{device} {
    createPipelineLayout(globalSetLayout);
    // createPipeline(renderPass); // Replaced by createPipelines
    createPipelines(renderPass);

    std::srand(std::time(0));
}

SimpleRenderSystem::~SimpleRenderSystem() {
    vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
    // Pipelines are unique_ptr and will be destroyed automatically
}

void SimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SimplePushConstantData);
    
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

// void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) { // Replaced by createPipelines
//     assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");
// 
//     PipelineConfigInfo pipelineConfig{};
//     Pipeline::defaultPipelineConfigInfo(pipelineConfig); 
//     pipelineConfig.renderPass = renderPass;
//     pipelineConfig.pipelineLayout = pipelineLayout;
//     pipeline = std::make_unique<Pipeline>(device, "shaders/simple_shader.vert.spv", "shaders/simple_shader.frag.spv", pipelineConfig);
// }

void SimpleRenderSystem::createPipelines(VkRenderPass renderPass) {
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    Pipeline::defaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;

    // Textured pipeline (original)
    texturedPipeline = std::make_unique<Pipeline>(device, "shaders/simple_shader.vert.spv", "shaders/simple_shader.frag.spv", pipelineConfig);

    // Vertex visualization pipeline
    // We might need to adjust pipelineConfig for point rendering if not handled by the shader alone
    // For example, if you want to ensure points are drawn:
    // pipelineConfig.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    // pipelineConfig.rasterizationInfo.polygonMode = VK_POLYGON_MODE_POINT; // If you want to see vertices of triangles as points
    // However, the vertex_highlight.vert shader uses gl_PointSize, which should work with triangle topology.
    // If issues arise, these settings are the first to check.
    PipelineConfigInfo wireframePipelineConfig = pipelineConfig; // Start with a copy
    // If your vertex_highlight shader is designed to work with point topology:
    // vertexVisPipelineConfig.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    // vertexVisPipelineConfig.bindingDescriptions = {Model::Vertex::getBindingDescription()}; // Or just position
    // vertexVisPipelineConfig.attributeDescriptions = {Model::Vertex::getAttributeDescriptions()[0]}; // Just position

    // Configure for wireframe rendering
    wireframePipelineConfig.rasterizationInfo.polygonMode = VK_POLYGON_MODE_LINE;
    wireframePipelineConfig.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Ensure we are drawing lines for triangles

    wireframePipeline = std::make_unique<Pipeline>(device, "shaders/wireframe.vert.spv", "shaders/wireframe.frag.spv", wireframePipelineConfig);
}


void SimpleRenderSystem::renderGameObjects(FrameInfo &frameInfo) {
    Pipeline* currentPipeline = nullptr;

    switch (static_cast<RenderMode>(config().getInt("render_mode"))) {
        case RenderMode::WIREFRAME:
            currentPipeline = wireframePipeline.get();
            break;
        case RenderMode::NORMAL_TEXTURED:
        default:
            currentPipeline = texturedPipeline.get();
            break;
    }

    if (!currentPipeline) {
        throw std::runtime_error("No pipeline selected for rendering!");
    }

    currentPipeline->bind(frameInfo.commandBuffer);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);

    for (auto& kv : frameInfo.gameObjects) {
        auto &obj = kv.second;

        if(obj->model == nullptr) continue;

        SimplePushConstantData push{};
        push.modelMatrix = obj->transform.mat4();
        push.normalMatrix = obj->transform.normalMatrix();

        vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData), &push);
        obj->model->bind(frameInfo.commandBuffer);
        obj->model->draw(frameInfo.commandBuffer);
    }
}
