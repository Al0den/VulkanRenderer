#pragma once

#include "window.hpp"
#include "device.hpp"
#include "renderer.hpp"
#include "game_object.hpp"
#include "descriptors.hpp"
#include "chunk_manager.hpp"

#include <vector>

namespace vkengine {

class App {
    public:
        App();
        ~App();

        App(const App &) = delete;
        App &operator=(const App &) = delete;

        static constexpr int WIDTH = 1900;
        static constexpr int HEIGHT = 1180;

        // Chunk view distance (in chunk units)
        static constexpr int CHUNK_VIEW_DISTANCE = 6;

        void run();

    private:
        void loadGameObjects();

        Window window{WIDTH, HEIGHT, "Vulkan"};
        Device device{window};
        Renderer renderer{window, device};
    
        std::unique_ptr<DescriptorPool> globalPool{};
        GameObject::Map gameObjects{};
        std::unique_ptr<ChunkManager> chunkManager{};
};

}
