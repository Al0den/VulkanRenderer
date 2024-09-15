#include "../include/app.hpp"
#include <cstdlib>

using namespace vkengine;

#include <iostream>

int main() {
    App app;

    try {
        app.run();
    } catch (const std::exception &e){
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
