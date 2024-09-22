#pragma once

#include "device.hpp"
#include "window.hpp"
#include "frame_info.hpp"

#include "../third-party/imgui/imgui.h"
#include "../third-party/imgui/imgui_impl_glfw.h"
#include "../third-party/imgui/imgui_impl_vulkan.h"


namespace vkengine {

static void check_vk_result(VkResult err) {
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

class Imgui {
public:
    Imgui(Window &window, Device &device, VkRenderPass renderPass, uint32_t imageCount);
    ~Imgui();
    void newFrame();
    void render(VkCommandBuffer commandBuffer);

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    void debugWindow(FrameInfo &frameInfo);


private:
    int numVertices = 0;
    int numIndices = 0;
    Device &device;

    VkDescriptorPool descriptorPool;
};
}
