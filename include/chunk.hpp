#pragma once

#include "game_object.hpp"
#include "model.hpp"
#include "hash.hpp"
#include "enums.hpp"
#include "perlin_noise.hpp"

#include <memory>
#include <array>
#include <vector>
#include <glm/glm.hpp>
#include <mutex>

namespace vkengine {

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
    
    Block() : type(BlockType::AIR) {}
    Block(BlockType t) : type(t) {}
};

struct TerrainSettings {
    PerlinNoise temperatureNoise;
    PerlinNoise humidityNoise;
    PerlinNoise elevationNoise;
    PerlinNoise riverNoise;
    PerlinNoise caveNoise;
    PerlinNoise oreNoise;

    // -- 2D Biomes Map -- 
    double temperatureFrequency = 0.001;
    double heatFrequency = 0.001;


    // -- Heightmap Generation --
    double elevFrequency = 0.1;
    int elevOctaves = 4;
    double elevPersistence = 0.5;
    double elevHeightScale = 100.0;
    double elevBaseHeight = 0.0;

    // -- River Carving -- 
    double riverFrequency = 0.001;
    double riverThreshold = 0.3;
    double riverBedHeight = CHUNK_SIZE * 0.25f;

    // -- Caves --
    double caveFrequency = 0.1;
    double caveThreshold = 0.6;

    // -- Soil -- 
    int baseSoilDepth = 3;
    double soilDepthVariation = 0.5;

    int maxHeight = -256;
    int minHeight = 256;
    
    TerrainSettings(uint64_t seed = 0) : 
        temperatureNoise(seed + 1), 
        humidityNoise(seed + 2),
        elevationNoise(seed + 3),
        riverNoise(seed + 4),
        caveNoise(seed + 5),
        oreNoise(seed + 6) {}
};

class Chunk {
public:
    // Constructor with a shared pointer to a game object that will represent this chunk
    Chunk(Device &device, std::shared_ptr<GameObject> gameObject);
    ~Chunk();

    void initialize();
    void generateTerrain();

    void fill(int x1, int y1, int z1, int x2, int y2, int z2, BlockType blockType);
    void setBlock(int x, int y, int z, BlockType blockType);
    Block getBlock(int x, int y, int z) const;
    bool isInBounds(int x, int y, int z) const;

    bool defaultTerrainGenerated() const { return flags & ChunkFlags::DEFAULT_TERRAIN_GENERATED; }
    bool meshGenerated() const { return flags & ChunkFlags::MESH_GENERATED; }
    bool upToDate() const { return flags & ChunkFlags::UP_TO_DATE; }

    void setMeshGenerated(bool generated);
    void setUpToDate(bool upToDate);

    void generateMesh();
    void generateGreedyMesh();
    void updateGameObject();
    
    std::shared_ptr<GameObject> getGameObject() const { return m_gameObject; }

    std::mutex m_mutex;

    ChunkCoord getChunkCoord() const;

    bool allNeighborsLoaded() const;

    std::array<std::shared_ptr<Chunk>, 6> m_neighbors{nullptr};

    void clearMesh();

    std::string serialize() const;
    void deserialize(const std::string& data);

private:
    struct VertexPosHash {
        size_t operator()(const glm::vec3& v) const {
            return std::hash<float>()(v.x) ^ 
                  (std::hash<float>()(v.y) << 1) ^ 
                  (std::hash<float>()(v.z) << 2);
        }
    };

    std::array<Block, CHUNK_VOLUME> m_blocks;
    
    std::shared_ptr<GameObject> m_gameObject;

    std::vector<Model::Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    int coordsToIndex(int x, int y, int z) const;

    void addBlockFace(int x, int y, int z, BlockType blockType, Direction direction);
    void processGreedyDirection(Direction direction, std::shared_ptr<Chunk> neighbor);
    void addGreedyFace(int normal, int u, int v, int width, int height, BlockType blockType, Direction direction, int normalAxis, int uAxis, int vAxis);

    Device &device;

    TerrainSettings settings{0};

    int flags = NONE;
};

} // namespace vkengine
