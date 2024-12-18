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
        GLFWwindow *getWindow() const { return window; }

        Window(const Window &) = delete;
        Window &operator=(const Window &) = delete;

        bool wasWindowResized() { return framebufferResized; }
        void resetWindowResizedFlag() { framebufferResized = false; }

        GLFWwindow *getGLFWWindow() { return window; }

    private:
        static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
        void initWindow();

        int width;
        int height;
        bool framebufferResized = false;

        std::string windowName;

        GLFWwindow *window;
};

}
