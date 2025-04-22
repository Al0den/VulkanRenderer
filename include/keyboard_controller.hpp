#pragma once

#include "game_object.hpp"
#include "config.hpp"

namespace vkengine {

class KeyboardController {
public:
    struct KeyMappings {
        int moveLeft = GLFW_KEY_A;
        int moveRight = GLFW_KEY_D;
        int moveUp = GLFW_KEY_SPACE;
        int moveDown = GLFW_KEY_C;
        int moveForward = GLFW_KEY_W;
        int moveBackward = GLFW_KEY_S;
        int lookDown = GLFW_KEY_DOWN;
        int lookUp = GLFW_KEY_UP;
        int lookLeft = GLFW_KEY_LEFT;
        int lookRight = GLFW_KEY_RIGHT;
    };

    void moveInPlaneXZ(GLFWwindow *window, std::shared_ptr<GameObject> gameObject, float deltaTime);
    
    KeyMappings keys{};
    float moveSpeed;
    float lookSpeed{1.5f};
};
}
