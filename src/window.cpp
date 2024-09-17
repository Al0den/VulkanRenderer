#include "../include/window.hpp"

using namespace vkengine;

Window::Window(int w, int h, std::string name) : width(w), height(h), windowName(name) {
    initWindow();
}

Window::~Window() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void closeCallback(GLFWwindow* window) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_W && action == GLFW_PRESS && mods == GLFW_MOD_SUPER)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}
void Window::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

    glfwSetWindowCloseCallback(getWindow(), closeCallback);
    glfwMakeContextCurrent(getWindow());

    glfwSetKeyCallback(window, keyCallback);
}

void Window::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface) {
    if(glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface");
    }
}

void Window::framebufferResizeCallback(GLFWwindow *window, int width, int height) {
    auto l_window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    l_window->framebufferResized = true;
    l_window->width = width;
    l_window->height = height;
}
