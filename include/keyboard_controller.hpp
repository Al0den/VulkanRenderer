#pragma once

#include "game_object.hpp"
#include "window.hpp"

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

        void moveInPlaneXZ(GLFWwindow *window, GameObject &gameObject, float deltaTime);
        
        KeyMappings keys{};
        float moveSpeed{3.f};
        float lookSpeed{1.5f};


};
}