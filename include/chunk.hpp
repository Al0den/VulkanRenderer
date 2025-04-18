#pragma once

#include "game_object.hpp"
#include "model.hpp"

#include <memory>
#include <array>
#include <vector>
#include <glm/glm.hpp>

namespace vkengine {

// Define block types
enum class BlockType {
    AIR,
    DIRT,
    GRASS,
    STONE,
    SAND,
    WATER,
    WOOD,
    LEAVES
    // Add more block types as needed
};

// Size constants for chunk dimensions
constexpr int CHUNK_SIZE = 16;
constexpr int CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

struct Block {
    BlockType type;
    
    // Default constructor creates an AIR block
    Block() : type(BlockType::AIR) {}
    
    // Constructor with specified block type
    Block(BlockType t) : type(t) {}
};

// Direction enum for block faces
enum class Direction {
    TOP,
    BOTTOM,
    FRONT,
    BACK,
    LEFT,
    RIGHT
};

class Chunk {
public:
    // Constructor with a shared pointer to a game object that will represent this chunk
    Chunk(Device &device, std::shared_ptr<GameObject> gameObject);
    ~Chunk();

    // Initialize the chunk with default blocks
    void initialize();
    
    // Generate terrain for the chunk
    void generateTerrain();
    
    // Fill a region of the chunk with a specific block type
    void fill(int x1, int y1, int z1, int x2, int y2, int z2, BlockType blockType);
    
    // Set a block at the specified coordinates
    void setBlock(int x, int y, int z, BlockType blockType);
    
    // Get a block at the specified coordinates
    Block getBlock(int x, int y, int z) const;
    
    // Check if coordinates are within chunk bounds
    bool isInBounds(int x, int y, int z) const;
    
    // Generate mesh for the chunk
    void generateMesh();
    
    // Update the chunk's game object
    void updateGameObject();
    
    // Get the game object associated with this chunk
    std::shared_ptr<GameObject> getGameObject() const { return m_gameObject; }

private:
    // 3D array of blocks
    std::array<Block, CHUNK_VOLUME> m_blocks;
    
    // Shared pointer to the game object representing this chunk
    std::shared_ptr<GameObject> m_gameObject;
    
    // Mesh data
    std::vector<Model::Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    
    // Flag to track if mesh needs regeneration
    bool m_modified = true;
    
    // Helper function to convert 3D coordinates to a 1D array index
    int coordsToIndex(int x, int y, int z) const;
    
    // Helper function to add a face to the mesh
    void addBlockFace(int x, int y, int z, BlockType blockType, Direction direction);
    
    // Device reference for creating models
    Device &device;
};

} // namespace vkengine
