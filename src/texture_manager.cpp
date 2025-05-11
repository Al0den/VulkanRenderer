#include "texture_manager.hpp"

#include "stb_image.h" // For loading images

#include <stdexcept>
#include <iostream> // For error reporting
#include <vector>
#include <cstring> // For memcpy

namespace vkengine {

// Constructor
TextureManager::TextureManager(Device& device)
    : device{device}, textureArrayImage{VK_NULL_HANDLE}, textureArrayImageMemory{VK_NULL_HANDLE}, textureArrayImageView{VK_NULL_HANDLE}, sampler{VK_NULL_HANDLE}, numLayers{0} {}

// Destructor
TextureManager::~TextureManager() {
    if (sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device.device(), sampler, nullptr);
    }
    // Clean up the single texture array image view
    if (textureArrayImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device.device(), textureArrayImageView, nullptr);
    }
    // Clean up the single texture array image and its memory
    if (textureArrayImage != VK_NULL_HANDLE) {
        vkDestroyImage(device.device(), textureArrayImage, nullptr);
    }
    if (textureArrayImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device.device(), textureArrayImageMemory, nullptr);
    }
}

// Private helper to create VkImage and allocate memory
void TextureManager::createImage(uint32_t width, uint32_t height, VkFormat format,
                                 VkImageTiling tiling, VkImageUsageFlags usage,
                                 VkMemoryPropertyFlags properties, VkImage& image,
                                 VkDeviceMemory& imageMemory, uint32_t arrayLayers) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = arrayLayers; // Use arrayLayers parameter
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    device.createImageWithInfo(imageInfo, properties, image, imageMemory);
}

void TextureManager::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView& imageView, VkImageViewType viewType, uint32_t layerCount) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = viewType;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = layerCount; 
    if (vkCreateImageView(device.device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture image view!");
    }
}

void TextureManager::createTextureSampler() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR; // Or VK_FILTER_NEAREST
    samplerInfo.minFilter = VK_FILTER_LINEAR; // Or VK_FILTER_NEAREST
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE; 
    // Query device features for max anisotropy
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(device.getPhysicalDevice(), &properties);
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; // Or VK_SAMPLER_MIPMAP_MODE_NEAREST
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f; // Adjust if using mipmaps
    if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture sampler!");
    }
}


void TextureManager::loadTextures() {
    const auto& textureConfigs = getGlobalTextureConfig();

    if (textureConfigs.empty()) {
        std::cout << "No textures configured. No textures will be loaded." << std::endl;
        numLayers = 0;
        // Optionally, create a default/dummy texture array if your application requires it.
        return;
    }

    numLayers = textureConfigs.size();
    std::vector<RawImage> loadedRawImages;
    loadedRawImages.reserve(numLayers);

    for (const auto& config : textureConfigs) {
        int texWidthStb, texHeightStb, texChannelsStb;
        unsigned char* pixels = stbi_load(config.path.c_str(), &texWidthStb, &texHeightStb, &texChannelsStb, STBI_rgb_alpha);
        if (!pixels) {
            throw std::runtime_error("Failed to load texture file: " + config.path + " - " + stbi_failure_reason());
        }
        if (static_cast<uint32_t>(texWidthStb) != TEXTURE_WIDTH || static_cast<uint32_t>(texHeightStb) != TEXTURE_HEIGHT) {
            stbi_image_free(pixels);
            throw std::runtime_error("Texture " + config.path + " has dimensions " +
                                     std::to_string(texWidthStb) + "x" + std::to_string(texHeightStb) +
                                     ", but expected " + std::to_string(TEXTURE_WIDTH) + "x" + std::to_string(TEXTURE_HEIGHT));
        }
        loadedRawImages.push_back({pixels, texWidthStb, texHeightStb, STBI_rgb_alpha, config.id});
    }
    
    if (loadedRawImages.empty()) {
         throw std::runtime_error("No images were successfully loaded, though configurations were present.");
    }

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(TEXTURE_WIDTH) * TEXTURE_HEIGHT * 4; // 4 for RGBA
    VkDeviceSize totalSize = imageSize * numLayers;

    Buffer stagingBuffer{
        device,
        totalSize,
        1, // instance count, for buffer not individual images
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
    
    stagingBuffer.map();
    unsigned char* mappedData = static_cast<unsigned char*>(stagingBuffer.getMappedMemory());

    for (size_t i = 0; i < numLayers; ++i) {
        memcpy(mappedData + (i * imageSize), loadedRawImages[i].pixels, imageSize);
        stbi_image_free(loadedRawImages[i].pixels); // Free CPU-side pixel data
    }
    stagingBuffer.unmap(); // Unmap after all images are copied

    loadedRawImages.clear();

    // Create a single VkImage for the texture array
    createImage(TEXTURE_WIDTH, TEXTURE_HEIGHT, VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                textureArrayImage, textureArrayImageMemory, numLayers); // Pass numLayers here

    VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();

    // Transition layout for the entire array
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = textureArrayImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = numLayers; // Apply to all layers
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Copy buffer to image, one layer at a time or in a batch if API supports
    // For simplicity, using VkBufferImageCopy for each layer.
    // More optimal: use a single vkCmdCopyBufferToImage with multiple regions if possible,
    // or ensure your copyBufferToImage can handle array layers directly.
    // Assuming device.copyBufferToImage is adapted or you use vkCmdCopyBufferToImage with VkBufferImageCopy regions.

    std::vector<VkBufferImageCopy> bufferCopyRegions;
    for (uint32_t i = 0; i < numLayers; ++i) {
        VkBufferImageCopy region{};
        region.bufferOffset = imageSize * i;
        region.bufferRowLength = 0; // Tightly packed
        region.bufferImageHeight = 0; // Tightly packed
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = i;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {TEXTURE_WIDTH, TEXTURE_HEIGHT, 1};
        bufferCopyRegions.push_back(region);
    }
    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.getBuffer(), textureArrayImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(bufferCopyRegions.size()), bufferCopyRegions.data());


    // Transition layout for shader access for the entire array
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    // barrier.subresourceRange.layerCount is already numLayers
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    device.endSingleTimeCommands(commandBuffer);

    // Create a single image view for the texture array
    createImageView(textureArrayImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, textureArrayImageView, VK_IMAGE_VIEW_TYPE_2D_ARRAY, numLayers);
    
    createTextureSampler(); // Sampler can be shared

    std::cout << "Loaded " << numLayers << " textures into a texture array." << std::endl;
}

} // namespace vkengine
