#include "../include/imgui.hpp"
#include "../include/config.hpp"
#include "../include/frame_info.hpp"
#include "../include/scope_timer.hpp"
// libs
// std
#include <stdexcept>
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>

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

void Imgui::showPerformanceTab() {
    // Access the global timer data
    const auto& timerData = GlobalTimerData::get();
    
    ImGui::Begin("Performance");
    
    if (ImGui::CollapsingHeader("Scope Timers", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Check if there are any timers
        bool hasTimers = timerData.begin() != timerData.end();
        
        if (hasTimers) {
            // Find the "global" timer to use as reference
            double globalTime = 0.0;
            std::vector<std::pair<std::string, double>> timerValues;
            
            // First pass: find global timer and collect all timer values
            for (const auto& timer : timerData) {
                // Convert the timer value to string first and then to double
                std::stringstream timerStream;
                timerStream << timer.second;
                std::string timerStr = timerStream.str();
                double timeValue;
                
                try {
                    // Extract the numerical part from the timer string
                    size_t pos;
                    timeValue = std::stod(timerStr, &pos);
                } catch (const std::exception& e) {
                    timeValue = 0.0; // Default if conversion fails
                }
                
                // Check if this is the global timer
                if (timer.first == "global") {
                    globalTime = timeValue;
                }
                
                timerValues.push_back({timer.first, timeValue});
            }
            
            // Table for organized display
            ImGui::BeginTable("TimersTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
            ImGui::TableSetupColumn("Timer ID", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("% of Global Timer", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableHeadersRow();
            
            // Second pass: display each timer's data with proportion relative to globa timer
            for (const auto& timerPair : timerValues) {
                const auto& timerName = timerPair.first;
                double timeValue = timerPair.second;
                
                // Calculate proportion relative to the global timer
                double proportion = 0.0;
                if (globalTime > 0.0) {
                    proportion = (timeValue / globalTime) * 100.0;
                }
                
                // Skip showing the globa timer itself in the proportion column
                bool isGlobalTimer = (timerName == "global");
                if (isGlobalTimer) {
                    continue;
                }
                
                ImGui::TableNextRow();
                
                // Timer ID column
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(timerName.c_str());
                
                // Time column with proper units
                ImGui::TableSetColumnIndex(1);
                std::stringstream timeStream;
                timeStream << std::fixed << std::setprecision(2);
                
                if (timeValue < 1000.0) {
                    timeStream << timeValue << " ns";
                } else if (timeValue < 1000000.0) {
                    timeStream << (timeValue / 1000.0) << " μs";
                } else if (timeValue < 1000000000.0) {
                    timeStream << (timeValue / 1000000.0) << " ms";
                } else {
                    timeStream << (timeValue / 1000000000.0) << " s";
                }
                
                ImGui::Text("%s", timeStream.str().c_str());
                
                // Proportion column with progress bar
                ImGui::TableSetColumnIndex(2);
                if (isGlobalTimer) {
                    ImGui::TextUnformatted("100.0%");
                } else {
                    std::stringstream propStream;
                    propStream << std::fixed << std::setprecision(1) << proportion << "%";
                    ImGui::ProgressBar(proportion / 100.0, ImVec2(-1, 0), propStream.str().c_str());
                }
            }
            ImGui::EndTable();
            
            // Display global timer reference
            std::stringstream globalTimeStr;
            globalTimeStr << std::fixed << std::setprecision(2);
            if (globalTime < 1000.0) {
                globalTimeStr << "Global timer: " << globalTime << " ns";
            } else if (globalTime < 1000000.0) {
                globalTimeStr << "Global timer: " << (globalTime / 1000.0) << " μs";
            } else if (globalTime < 1000000000.0) {
                globalTimeStr << "Global timer: " << (globalTime / 1000000.0) << " ms";
            } else {
                globalTimeStr << "Global timer: " << (globalTime / 1000000000.0) << " s";
            }
            ImGui::Text("%s", globalTimeStr.str().c_str());
        } else {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "No timer data available");
        }
    }
    
    ImGui::End();
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
        ImGui::Begin("Meshing");
        
        // Meshing technique selection
        static const char* meshingTechniques[] = { "Regular Meshing", "Greedy Meshing" };
        static const char* renderMethods[] = { "UV", "Wireframe", "Texture", "Color"};
        static int currentRenderMethod = config().getInt("render_mode");
        static int currentMeshingTechnique = config().getInt("meshing_technique");
        
        ImGui::Text("Meshing Technique");
        if (ImGui::Combo("##MeshingTechnique", &currentMeshingTechnique, meshingTechniques, IM_ARRAYSIZE(meshingTechniques))) {
            config().setInt("meshing_technique", currentMeshingTechnique);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Meshing technique changed. New chunks will use the selected method.");
            frameInfo.chunkManager->regenerateEntireMesh();
        }
        ImGui::Text("Render Method");
        if (ImGui::Combo("##RenderTechnique", &currentRenderMethod, renderMethods, IM_ARRAYSIZE(renderMethods))) {
            config().setInt("render_mode", currentRenderMethod);
        }
        
        ImGui::Text("Statistics");
        ImGui::Text("Vertices: %d", numVertices);
        ImGui::Text("Indices: %d", numIndices);
        ImGui::Text("Triangles: %d", numIndices / 3);
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        
        ImGui::End();
    }
    {
        ImGui::Begin("World");
        if (ImGui::Button("Save World")) {
            // Save world logic here
            std::string worldData = frameInfo.chunkManager->serialize();
            std::ofstream outFile("./data/world_data.txt");
            if (outFile.is_open()) {
                outFile << worldData;
                outFile.close();
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "World saved successfully!");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to save world.");
            }
        }
        static const char* generateChunks[] = { "False", "True"};
        static int generateChunksCurrent = frameInfo.chunkManager->flags & ChunkManagerFlags::GENERATE_CHUNKS;
        ImGui::Text("Generating Chunks");

        if (ImGui::Combo("##Generate Chunks", &generateChunksCurrent, generateChunks, IM_ARRAYSIZE(generateChunks))) {
            if (generateChunksCurrent) {
                frameInfo.chunkManager->flags |= ChunkManagerFlags::GENERATE_CHUNKS;
            } else {
                frameInfo.chunkManager->flags &= ~ChunkManagerFlags::GENERATE_CHUNKS;
            }
        }

        if (ImGui::Button("Load Map")) {
            // Load world logic here
            std::ifstream inFile("./data/world_data.txt");
            if (inFile.is_open()) {
                std::string worldData((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
                frameInfo.chunkManager->deserialize(worldData);
                inFile.close();
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "World loaded successfully!");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Failed to load world.");
            }

            frameInfo.chunkManager->flags &= ~ChunkManagerFlags::GENERATE_CHUNKS;
        }
        ImGui::End();
    }
    
    // Show the performance tab with scope timer information
    showPerformanceTab();
}
}
