#pragma once
#include "device.hpp"
#include "window.hpp"
// libs

#include "../third-party/imgui/imgui.h"
#include "../third-party/imgui/imgui_impl_glfw.h"
#include "../third-party/imgui/imgui_impl_vulkan.h"
// std
#include <stdexcept>

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
    // Example state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    void debugWindow();

private:
    Device &device;
    // We haven't yet covered descriptor pools in the tutorial series
    // so I'm just going to create one for just imgui and store it here for now.
    // maybe its preferred to have a separate descriptor pool for imgui anyway,
    // I haven't looked into imgui best practices at all.
    VkDescriptorPool descriptorPool;
};
}
