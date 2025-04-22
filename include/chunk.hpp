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

struct ChunkCoord {
    int x;
    int y;
    int z;
    
    bool operator==(const ChunkCoord& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
    
    // Hash function for ChunkCoord to use in unordered_map
    struct Hash {
        std::size_t operator()(const ChunkCoord& coord) const {
            return std::hash<int>()(coord.x) ^ 
                   (std::hash<int>()(coord.y) << 1) ^
                   (std::hash<int>()(coord.z) << 2);
        }
    };
};

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

    bool defaultTerrainGenerated() const { return m_defaultTerrainGenerated; }
    bool meshGenerated() const { return m_meshGenerated; }
    bool upToDate() const { return m_upToDate; }
    bool isUpdated() const { return m_updated; }

    void setMeshGenerated(bool generated) { m_meshGenerated = generated; }
    
    // Generate mesh for the chunk
    void generateMesh(const std::unordered_map<ChunkCoord, std::shared_ptr<Chunk>, ChunkCoord::Hash> &chunks);
    
    // Generate optimized mesh using greedy meshing algorithm
    void generateGreedyMesh(const std::unordered_map<ChunkCoord, std::shared_ptr<Chunk>, ChunkCoord::Hash> &chunks);
    
    // Update the chunk's game object
    void updateGameObject();
    
    // Get the game object associated with this chunk
    std::shared_ptr<GameObject> getGameObject() const { return m_gameObject; }

    bool m_updated = false;

private:
    // Structure to hash vertex positions for the vertex cache
    struct VertexPosHash {
        size_t operator()(const glm::vec3& v) const {
            return std::hash<float>()(v.x) ^ 
                  (std::hash<float>()(v.y) << 1) ^ 
                  (std::hash<float>()(v.z) << 2);
        }
    };

    // 3D array of blocks
    std::array<Block, CHUNK_VOLUME> m_blocks;
    
    // Shared pointer to the game object representing this chunk
    std::shared_ptr<GameObject> m_gameObject;
    
    // Mesh data
    std::vector<Model::Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    
    // Vertex cache for sharing vertices between faces
    std::unordered_map<glm::vec3, uint32_t, VertexPosHash> m_vertexCache;
    
    // Flag to track if mesh needs regeneration
    bool m_defaultTerrainGenerated = false;
    bool m_meshGenerated = false;
    bool m_upToDate = false;

    
    // Helper function to convert 3D coordinates to a 1D array index
    int coordsToIndex(int x, int y, int z) const;
    
    // Helper function to add a face to the mesh
    void addBlockFace(int x, int y, int z, BlockType blockType, Direction direction);
    
    // Helper function for greedy meshing to add a merged face
    void addGreedyFace(float x, float y, float z, float width, float height, 
                       BlockType blockType, Direction direction, 
                       const glm::vec3& normal, bool flipWinding);
    
    // Device reference for creating models
    Device &device;
};

} // namespace vkengine
