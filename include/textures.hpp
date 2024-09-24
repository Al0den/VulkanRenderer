#pragma once

#include "device.hpp"

#include <string>
#include <unordered_map>

namespace vkengine {

class Texture {
public:
    Texture(Device &device, const std::string &filepath);
    ~Texture();

    VkSampler getSampler() const { return sampler; }
    VkImageView getImageView() const { return imageView; }
    VkImageLayout getImageLayout() const { return imageLayout; }

    Texture(const Texture &) = delete;
    Texture &opeartor(const Texture &) = delete;
    Texture(Texture &&) = delete;
    Texture &operator=(Texture &&) = delete;

    VkDescriptorImageInfo imageInfo;
private:
    void transitionImageLayout(VkImageLayout oldImageLayout, VkImageLayout newLayout);

    Device &device;
    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;
    VkSampler sampler;
    VkFormat imageFormat;
    VkImageLayout imageLayout;

};

class TextureManager {

public:

    TextureManager(Device &device) : device(device) {}

    const std::shared_ptr<Texture> loadTexture(const std::string& filepath) {
        if(textures.find(filepath) == textures.end()) {
            textures[filepath] = std::make_shared<Texture>(device, filepath);
        }
        return textures[filepath];
    }

private:
    Device &device;

    std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
};


}
