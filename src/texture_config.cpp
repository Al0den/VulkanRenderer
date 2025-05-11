
#include "texture_config.hpp"

namespace vkengine {

// Define your texture configurations here.
// This is where you map each BlockId to its texture file path.
static const std::vector<TextureConfigEntry> globalTextureConfig = {
    {BlockType::AIR,        "textures/grass.png"},
    {BlockType::DIRT,       "textures/dirt.png"},
    {BlockType::GRASS,      "textures/grass.png"},    
    {BlockType::STONE,      "textures/stone.png"},
    {BlockType::WATER,      "textures/water.png"}, 
};

const std::vector<TextureConfigEntry>& getGlobalTextureConfig() {
    return globalTextureConfig;
}

// Optional: Implementation for BlockId to string mapping (for debugging)
/*
static const std::unordered_map<BlockId, std::string> blockIdToStringMap = {
    {BlockId::UNKNOWN,     "UNKNOWN"},
    {BlockId::DIRT,        "DIRT"},
    {BlockId::DIRT_TOP,    "DIRT_TOP"},
    {BlockId::DIRT_BOTTOM, "DIRT_BOTTOM"},
    {BlockId::GRASS,       "GRASS"},
    {BlockId::GRASS_TOP,   "GRASS_TOP"},
    {BlockId::GRASS_SIDE,  "GRASS_SIDE"},
    {BlockId::STONE,       "STONE"},
    {BlockId::WATER,       "WATER"},
    // Add mappings for other BlockIds
};

const std::unordered_map<BlockId, std::string>& getBlockIdToStringMap() {
    return blockIdToStringMap;
}
*/

} // namespace vre
