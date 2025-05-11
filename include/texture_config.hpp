\
#pragma once

#include "chunk.hpp"

#include <string>
#include <vector>
#include <unordered_map>

namespace vkengine {

constexpr uint32_t MAX_TEXTURE_UNITS = 16; // Maximum number of textures in the descriptor array

struct TextureConfigEntry {
    BlockType id;
    std::string path; // File path to the texture
};

// Function to get the global texture configuration
// The actual configuration will be defined in texture_config.cpp
const std::vector<TextureConfigEntry>& getGlobalTextureConfig();

// Optional: For easy lookup if BlockId needs to be stringified (e.g., for debugging)
// const std::unordered_map<BlockId, std::string>& getBlockIdToStringMap();

} // namespace vre
