#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <vulkan/vulkan.h>


namespace vkengine {

class Window {
    public:
        Window(int width, int height, std::string windowName);
        ~Window();

        bool shouldClose() { return glfwWindowShouldClose(window); }
        void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);
        VkExtent2D getExtent() { return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }

        Window(const Window &) = delete;
        Window &operator=(const Window &) = delete;

    private:
        void initWindow();

        const int width;
        const int height;

        std::string windowName;

        GLFWwindow *window;
};

}
