#pragma once

#include "device.hpp"
#include "buffer.hpp" // If your Buffer class is used for staging
#include "texture_config.hpp" // Include the new texture config header

#include <string>
#include <unordered_map>
#include <vector>

namespace vkengine {

class TextureManager {
public:
    TextureManager(Device& device);
    ~TextureManager();

    // Delete copy constructor and assignment operator
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    /**
     * @brief Loads textures based on the global texture configuration into a texture array.
     * All textures are expected to be 512x512.
     */
    void loadTextures();

    VkImageView getTextureArrayImageView() const { return textureArrayImageView; }
    VkSampler getSampler() const { return sampler; }
    size_t getTextureCount() const { return numLayers; } // Represents the number of layers in the array

private:
    Device& device;

    VkImage textureArrayImage;
    VkDeviceMemory textureArrayImageMemory;
    VkImageView textureArrayImageView;
    VkSampler sampler; // A single sampler can be used for all textures if sampling parameters are the same
    size_t numLayers = 0; // Stores the number of loaded textures/layers

    // RawImage struct might still be useful for loading individual images before GPU upload
    struct RawImage {
        unsigned char* pixels;
        int width;
        int height;
        int channels;
        BlockType blockId; // Store BlockId
    };

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t arrayLayers = 1);
    void createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView& imageView, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, uint32_t layerCount = 1);
    void createTextureSampler();


    // singleTexWidth and singleTexHeight are fixed at 512x512, can be consts or defined directly
    static constexpr uint32_t TEXTURE_WIDTH = 512;
    static constexpr uint32_t TEXTURE_HEIGHT = 512;

    // textureUVMap is no longer needed
};

} // namespace vkengine