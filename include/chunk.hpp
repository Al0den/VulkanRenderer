#pragma once

#include "game_object.hpp"
#include "model.hpp"

#include <memory>
#include <array>
#include <vector>
#include <glm/glm.hpp>
#include <mutex>

namespace vkengine {

// Define block types
enum class BlockType {
    AIR,
    DIRT,
    GRASS,
    HGRASS, // new high grass block type
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

#include <cstdint>
#include <cstddef>

// ——— Bit-interleaving to make a 3D Morton code ———
static inline uint64_t expandBits(uint32_t v) {
    uint64_t x = v;
    x = (x | (x << 16)) & 0x0000FFFF0000FFFFULL;
    x = (x | (x <<  8)) & 0x00FF00FF00FF00FFULL;
    x = (x | (x <<  4)) & 0x0F0F0F0F0F0F0F0FULL;
    x = (x | (x <<  2)) & 0x3333333333333333ULL;
    x = (x | (x <<  1)) & 0x5555555555555555ULL;
    return x;
}

static inline uint64_t morton3D(uint32_t x, uint32_t y, uint32_t z) {
    // interleave bits: …z2y2x2 z1y1x1 z0y0x0
    return (expandBits(x) << 2)
         | (expandBits(y) << 1)
         |  expandBits(z);
}

// ——— SplitMix64 mixer for avalanche diffusion ———
static inline uint64_t splitmix64(uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x  = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x  = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

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
            uint64_t key = morton3D(
                static_cast<uint32_t>(coord.x),
                static_cast<uint32_t>(coord.y),
                static_cast<uint32_t>(coord.z)
            );
            return static_cast<std::size_t>(splitmix64(key));
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
    void generateMesh();
    
    // Generate optimized mesh using greedy meshing algorithm
    void generateGreedyMesh();
    
    // Update the chunk's game object
    void updateGameObject();
    
    // Get the game object associated with this chunk
    std::shared_ptr<GameObject> getGameObject() const { return m_gameObject; }

    bool m_updated = false;

    std::mutex m_mutex;

    ChunkCoord getChunkCoord() const {
        return { static_cast<int>(m_gameObject->transform.translation.x / CHUNK_SIZE),
                 static_cast<int>(m_gameObject->transform.translation.y / CHUNK_SIZE),
                 static_cast<int>(m_gameObject->transform.translation.z / CHUNK_SIZE) };
    }

    bool allNeighborsLoaded() const {
        for (const auto& neighbor : m_neighbors) {
            if (!neighbor) return false;
        }
        return true;
    }

    std::array<std::shared_ptr<Chunk>, 6> m_neighbors{nullptr};

    void clearMesh() {
        m_vertices.clear();
        m_indices.clear();
        m_vertexCache.clear();
        m_meshGenerated = false;
    }

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
    
    // Helper methods for greedy meshing
    void processGreedyDirection(Direction direction, std::shared_ptr<Chunk> neighbor);
    void addGreedyFace(int normal, int u, int v, int width, int height, BlockType blockType, Direction direction, 
                       int normalAxis, int uAxis, int vAxis);
    
    // Device reference for creating models
    Device &device;

    
};

} // namespace vkengine
