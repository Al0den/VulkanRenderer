#pragma once

#include "model.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <unordered_map>

namespace vkengine {

struct TransformComponent {
    glm::vec3 translation{};
    glm::vec3 scale{1.f, 1.f, 1.f};
    glm::vec3 rotation{};

    glm::mat4 mat4();
    glm::mat3 normalMatrix();
};

class GameObject {
    public:
        using id_t = unsigned int;
        using Map = std::unordered_map<id_t, std::shared_ptr<GameObject>>;

        static std::shared_ptr<GameObject> createGameObject() { 
            static id_t currentId = 0;
            return std::make_shared<GameObject>(currentId++); 
        }
        
        id_t getId() const { return id; }
        
        GameObject(const GameObject &) = delete;
        GameObject &operator=(const GameObject&) = delete;
        GameObject(GameObject&&) = default;
        GameObject &operator=(GameObject &&) = default;

        glm::vec3 color{};

        TransformComponent transform{};

        std::shared_ptr<Model> model{};

        VkDescriptorSet descriptorSet; //Undefined if texture == nullptr;

    public:
        GameObject(id_t objId) : id(objId) {}
        
    private:
    id_t id;
};

}
