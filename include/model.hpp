#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FRCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>
#include <memory>
#include <unordered_set>

#include "device.hpp"
#include "buffer.hpp"

namespace vkengine {

class Model {
    public:
        struct Vertex {
            glm::vec3 position{};
            glm::vec3 color{};
            glm::vec3 normal{};
            glm::vec2 uv{};
            uint32_t block_type;


            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

            bool operator==(const Vertex &other) const {
                return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
            }

        };

        struct Builder {
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;

            void loadModel(const std::string &filepath);
        };

        Model(Device &device, const Model::Builder &builder);
        ~Model();

        Model(const Model &) = delete;
        Model &operator=(const Model &) = delete;

        static std::unique_ptr<Model> createModelFromFile(Device &device, const std::string &filepath);

        void bind(VkCommandBuffer commandBuffer);
        void draw(VkCommandBuffer commandBuffer);

        int getVertexCount() const {
            return vertexCount;
        }
        int getIndexCount() const {
            if(hasIndexBuffer) {
                return indexCount;
            } else {
                return 0;
            }
        }

    private:
        void createVertexBuffers(const std::vector<Vertex> &vertices);
        void createIndexBuffer(const std::vector<uint32_t> &indices);

        Device &device;

        std::unique_ptr<Buffer> vertexBuffer;
        uint32_t vertexCount = 0;

        bool hasIndexBuffer = false;
        std::unique_ptr<Buffer> indexBuffer;
        uint32_t indexCount;
};

}
