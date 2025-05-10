\
#pragma once

#include "device.hpp"
#include <string>
#include <vulkan/vulkan.h>
#include <memory> // Required for std::unique_ptr if we go that route, but for now direct members

namespace vkengine {

class Texture {
public:
    Texture(Device &device, const std::string &filepath);
    ~Texture();

    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;

    VkImageView getImageView() const { return textureImageView; }
    VkSampler getSampler() const { return textureSampler; }
    VkDescriptorImageInfo getDescriptorInfo() const;


private:
    void createTextureImage(const std::string &filepath);
    void createTextureImageView();
    void createTextureSampler();

    // Helper functions (can be static or part of Device utils if preferred)
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    Device &vkDevice; // Renamed to avoid conflict with class name Device

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    uint32_t mipLevels{1}; // For now, no mipmapping
    VkFormat imageFormat; // Store the image format
};

} // namespace vkengine
