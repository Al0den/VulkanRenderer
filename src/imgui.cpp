#include "../include/imgui.hpp"
#include "../include/config.hpp"
#include "../include/frame_info.hpp"
// libs
// std
#include <stdexcept>
namespace vkengine {
// ok this just initializes imgui using the provided integration files. So in our case we need to
// initialize the vulkan and glfw imgui implementations, since that's what our engine is built
// using.
Imgui::Imgui(Window &window, Device &device, VkRenderPass renderPass, uint32_t imageCount) : device{device} {
    VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                         {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                         {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                         {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                         {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                         {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    if (vkCreateDescriptorPool(device.device(), &pool_info, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up descriptor pool for imgui");
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan(window.getGLFWWindow(), true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = device.getInstance();
    init_info.PhysicalDevice = device.getPhysicalDevice();
    init_info.Device = device.device();
    init_info.QueueFamily = device.getGraphicsQueueFamily();
    init_info.Queue = device.graphicsQueue();
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = descriptorPool;
    init_info.Allocator = VK_NULL_HANDLE;
    init_info.MinImageCount = 2;
    init_info.ImageCount = imageCount;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, renderPass);
    auto commandBuffer = device.beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
    device.endSingleTimeCommands(commandBuffer);
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}
Imgui::~Imgui() {
    vkDestroyDescriptorPool(device.device(), descriptorPool, nullptr);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
void Imgui::newFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Imgui::render(VkCommandBuffer commandBuffer) {
    ImGui::Render();
    ImDrawData *drawdata = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(drawdata, commandBuffer);
}

void Imgui::updateMeshStats(FrameInfo& frameInfo) {
    // Get current time
    float currentTime = ImGui::GetTime();
    
    // Update stats if it's time to do so
    if (currentTime - lastUpdateTime >= updateInterval) {
        numIndices = 0;
        numVertices = 0;
        for(auto& obj : frameInfo.gameObjects) {
            if(obj.second->model == nullptr) continue;
            numVertices += obj.second->model->getVertexCount();
            numIndices += obj.second->model->getIndexCount();
        }
        
        // Update the last update time
        lastUpdateTime = currentTime;
    }
}

void Imgui::debugWindow(FrameInfo& frameInfo) {
    // Automatically update mesh statistics at regular intervals
    updateMeshStats(frameInfo);
    
    {
        static float f = 0.0f;
        static int counter = 0;
        ImGui::Begin("Debug Window");           
        ImGui::Text("%d Game Objects", (int)frameInfo.gameObjects.size());
        ImGui::Text("x: %.2f, y: %.2f, z: %.2f", frameInfo.camera.getPosition().x, frameInfo.camera.getPosition().y, frameInfo.camera.getPosition().z);
        static float speed = config().getFloat("player_speed");
        if (ImGui::SliderFloat("Speed", &speed, 10.0f, 120.0f, "%.1f°")) {
            config().setFloat("player_speed", speed);
            
        }
        ImGui::End();
    }

    // Create settings window with FOV slider and render distance controls
    {
        ImGui::Begin("Settings");
        
        // FOV slider
        static float fov = config().getFloat("fov"); // Default FOV value
        ImGui::Text("Field of View");
        if (ImGui::SliderFloat("FOV", &fov, 30.0f, 120.0f, "%.1f°")) {
            // Update the config when FOV changes
            config().setFloat("fov", fov);
        }
        
        // Render distance controls
        static int renderDistance = config().getInt("render_distance"); // Default render distance
        ImGui::Text("Render Distance: %d chunks", renderDistance);
        if (ImGui::Button("Decrease")) {
            if (renderDistance > 1) {
                renderDistance--;
                config().setInt("render_distance", renderDistance);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Increase")) {
            if (renderDistance < 16) {
                renderDistance++;
                config().setInt("render_distance", renderDistance);
            }
        }
        
        ImGui::End();
    }
    {
        ImGui::Begin("Performance");
        
        // Meshing technique selection
        static const char* meshingTechniques[] = { "Regular Meshing", "Greedy Meshing" };
        static int currentMeshingTechnique = config().getInt("meshing_technique");
        
        ImGui::Text("Meshing Technique");
        if (ImGui::Combo("##MeshingTechnique", &currentMeshingTechnique, meshingTechniques, IM_ARRAYSIZE(meshingTechniques))) {
            config().setInt("meshing_technique", currentMeshingTechnique);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Meshing technique changed. New chunks will use the selected method.");
            frameInfo.chunkManager->regenerateAllMeshes();
        }
        
        ImGui::Text("Performance Statistics");
        ImGui::Text("Vertices: %d", numVertices);
        ImGui::Text("Indices: %d", numIndices);
        ImGui::Text("Triangles: %d", numIndices / 3);
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        
        ImGui::End();
    }
}
}
