#include "../../include/systems/simple_render_system.hpp"
#include "../../include/config.hpp" // Added for g_currentRenderMode
#include "../../include/scope_timer.hpp" // Added for texture configuration

#include <stdexcept>
#include <cstdlib>
#include <ctime>

#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

using namespace vkengine;
using ScopeTimer = GlobalTimerData::ScopeTimer;

struct SimplePushConstantData {
    glm::mat4 modelMatrix{1.f};
    glm::mat4 normalMatrix{1.f};
};

SimpleRenderSystem::SimpleRenderSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : device{device} {
    textureSetLayout = DescriptorSetLayout::Builder(device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();
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
    
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout, textureSetLayout->getDescriptorSetLayout()}; // Added textureSetLayout

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

void SimpleRenderSystem::createPipelines(VkRenderPass renderPass) {
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    Pipeline::defaultPipelineConfigInfo(pipelineConfig);
    pipelineConfig.renderPass = renderPass;
    pipelineConfig.pipelineLayout = pipelineLayout;

    // Textured pipeline (original uvPipeline)
    uvPipeline = std::make_unique<Pipeline>(device, "shaders/uv_shader.vert.spv", "shaders/uv_shader.frag.spv", pipelineConfig);
    colorPipeline = std::make_unique<Pipeline>(device, "shaders/color_shader.vert.spv", "shaders/color_shader.frag.spv", pipelineConfig);

    // Texture Atlas Pipeline
    // Assumes Model::Vertex::getAttributeDescriptions() includes the 'inBlockType' attribute
    texturePipeline = std::make_unique<Pipeline>(device, "shaders/texture_shader.vert.spv", "shaders/texture_shader.frag.spv", pipelineConfig);
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
        case RenderMode::TEXTURE: // New case for texture atlas rendering
            currentPipeline = texturePipeline.get();
            break;
        case RenderMode::COLOR:
            currentPipeline = colorPipeline.get();
            break;
        case RenderMode::UV:
        default:
            currentPipeline = uvPipeline.get();
            break;
    }

    if (!currentPipeline) {
        throw std::runtime_error("No pipeline selected for rendering!");
    }

    currentPipeline->bind(frameInfo.commandBuffer);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &frameInfo.globalDescriptorSet, 0, nullptr);
    if (static_cast<RenderMode>(config().getInt("render_mode")) == RenderMode::TEXTURE) {
        std::shared_ptr<TextureManager> textureManager = frameInfo.textureManager;
        if (textureManager && textureManager->getTextureArrayImageView() != VK_NULL_HANDLE) {
            // Check if the descriptor set needs to be created or updated
            // This is a simplified check; you might need a more robust way to track if the texture changed
            if (textureDescriptorSet == VK_NULL_HANDLE) { 
                VkDescriptorImageInfo imageInfo{};
                imageInfo.sampler = textureManager->getSampler();
                imageInfo.imageView = textureManager->getTextureArrayImageView();
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                bool buildSuccess = DescriptorWriter(*textureSetLayout, *frameInfo.globalPool) // Use globalPool
                    .writeImage(0, &imageInfo)
                    .build(textureDescriptorSet);
                if (!buildSuccess) {
                    throw std::runtime_error("Failed to build texture descriptor set");
                }
                std::cout << "Texture descriptor set bound successfully." << std::endl;
            }
            // Ensure textureDescriptorSet is valid before binding
            if (textureDescriptorSet != VK_NULL_HANDLE) {
              
                vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &textureDescriptorSet, 0, nullptr);
                
            } else {
                 throw std::runtime_error("Texture descriptor set is null even after attempting to build");
            }
        } else {
            throw std::runtime_error("TextureManager is null or texture array image view is not created");
        }
    }

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
