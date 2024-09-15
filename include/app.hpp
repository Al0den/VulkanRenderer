#pragma once

#include "window.hpp"
#include "device.hpp"
#include "renderer.hpp"
#include "game_object.hpp"

#include <memory>
#include <vector>

namespace vkengine {

class App {
    public:
        App();
        ~App();

        App(const App &) = delete;
        App &operator=(const App &) = delete;

        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 600;

        void run();

    private:
        void loadGameObjects();

        Window window{WIDTH, HEIGHT, "Vulkan"};
        Device device{window};
        Renderer renderer{window, device};

        std::vector<GameObject> gameObjects;
};

}
