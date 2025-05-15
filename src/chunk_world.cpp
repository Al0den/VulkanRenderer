#include "chunk.hpp"
#include "game_object.hpp"
#include "perlin_noise.hpp"
#include <algorithm> // added for std::clamp
#include <iostream> // added for std::cout
#include <string>
#include <sstream>

namespace vkengine {

Chunk::Chunk(Device &deviceRef, std::shared_ptr<GameObject> gameObject) 
    : device{deviceRef}, m_gameObject(gameObject) {
    initialize();
}

Chunk::~Chunk() {
}

void Chunk::initialize() {
    fill(0, 0, 0, CHUNK_SIZE - 1, CHUNK_SIZE - 1, CHUNK_SIZE - 1, BlockType::AIR);
}


void Chunk::generateTerrain() {
    // Basic elevation-based terrain using Perlin noise
    ChunkCoord coord = getChunkCoord();
    int worldOffsetX = coord.x * CHUNK_SIZE;
    int worldOffsetZ = coord.z * CHUNK_SIZE;

    for (int x = 0; x < CHUNK_SIZE; ++x) {
        for (int z = 0; z < CHUNK_SIZE; ++z) {
            // Get elevation from perlin noise.
            double nx = (worldOffsetX + x) * settings.elevFrequency;
            double nz = (worldOffsetZ + z) * settings.elevFrequency;
            double e = settings.elevationNoise.octaveNoise(nx, nz, settings.elevOctaves, settings.elevPersistence);
            int height = static_cast<int>(e * settings.elevHeightScale + settings.elevBaseHeight);
            height = std::clamp(height, 0, CHUNK_SIZE - 1);

            // Fill column by global world height
            for (int y = 0; y < CHUNK_SIZE; ++y) {
                // account for negative-y-up: flip local y so y=0 is top
                int worldY = coord.y * CHUNK_SIZE + (CHUNK_SIZE - 1 - y);
                if (worldY < height - settings.baseSoilDepth) {
                    setBlock(x, y, z, BlockType::STONE);
                } else if (worldY < height) {
                    setBlock(x, y, z, BlockType::DIRT);
                } else if (worldY == height) {
                    setBlock(x, y, z, BlockType::GRASS);
                } else {
                    setBlock(x, y, z, BlockType::AIR);
                }
            }
        }
    }

    flags |= ChunkFlags::DEFAULT_TERRAIN_GENERATED;
    flags &= ~ChunkFlags::MESH_GENERATED;
    flags &= ~ChunkFlags::UP_TO_DATE;
}

void Chunk::fill(int x1, int y1, int z1, int x2, int y2, int z2, BlockType blockType) {
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);
    if (z1 > z2) std::swap(z1, z2);
    
    x1 = std::max(0, std::min(x1, CHUNK_SIZE - 1));
    y1 = std::max(0, std::min(y1, CHUNK_SIZE - 1));
    z1 = std::max(0, std::min(z1, CHUNK_SIZE - 1));
    x2 = std::max(0, std::min(x2, CHUNK_SIZE - 1));
    y2 = std::max(0, std::min(y2, CHUNK_SIZE - 1));
    z2 = std::max(0, std::min(z2, CHUNK_SIZE - 1));
    
    for (int x = x1; x <= x2; x++) {
        for (int y = y1; y <= y2; y++) {
            for (int z = z1; z <= z2; z++) {
                setBlock(x, y, z, blockType);
            }
        }
    }
    
    flags &= ~ChunkFlags::MESH_GENERATED;
}

void Chunk::setBlock(int x, int y, int z, BlockType blockType) {
    if (isInBounds(x, y, z)) {
        int index = coordsToIndex(x, y, z);
        
        if (m_blocks[index].type != blockType) {
            m_blocks[index].type = blockType;
            flags &= ~ChunkFlags::MESH_GENERATED;
        }
    }
}

Block Chunk::getBlock(int x, int y, int z) const {
    if (isInBounds(x, y, z)) {
        int index = coordsToIndex(x, y, z);
        return m_blocks[index];
    }
    
    return Block(BlockType::AIR);
}

bool Chunk::isInBounds(int x, int y, int z) const {
    return (x >= 0 && x < CHUNK_SIZE &&
            y >= 0 && y < CHUNK_SIZE &&
            z >= 0 && z < CHUNK_SIZE);
}

int Chunk::coordsToIndex(int x, int y, int z) const {
    return x + (y * CHUNK_SIZE) + (z * CHUNK_SIZE * CHUNK_SIZE);
}

std::string Chunk::serialize() const {
    std::string out;
    // reserve header + worst-case RLE (2 bytes per block)
    out.reserve(3 * sizeof(int32_t) + 2 * m_blocks.size());

    // 1) Write X,Y,Z as 32-bit ints
    int32_t xi = static_cast<int32_t>(m_gameObject->transform.translation.x);
    int32_t yi = static_cast<int32_t>(m_gameObject->transform.translation.y);
    int32_t zi = static_cast<int32_t>(m_gameObject->transform.translation.z);
    out.append(reinterpret_cast<const char*>(&xi), sizeof(xi));
    out.append(reinterpret_cast<const char*>(&yi), sizeof(yi));
    out.append(reinterpret_cast<const char*>(&zi), sizeof(zi));

    // 2) RLE-encode block types
    if (!m_blocks.empty()) {
        uint8_t runType  = uint8_t(m_blocks[0].type);
        uint8_t runCount = 1;
        for (size_t i = 1; i < m_blocks.size(); ++i) {
            uint8_t t = uint8_t(m_blocks[i].type);
            if (t == runType && runCount < 255) {
                ++runCount;
            } else {
                out.push_back(static_cast<char>(runType));
                out.push_back(static_cast<char>(runCount));
                runType  = t;
                runCount = 1;
            }
        }
        // flush final run
        out.push_back(static_cast<char>(runType));
        out.push_back(static_cast<char>(runCount));
    }

    return out;
}

void Chunk::deserialize(const std::string& in) {
    // 1) Must be at least 12 bytes for X,Y,Z
    constexpr size_t HEADER = 3 * sizeof(int32_t);
    if (in.size() < HEADER)
        throw std::runtime_error("Chunk data too short");

    // 2) Read X,Y,Z
    int32_t xi, yi, zi;
    std::memcpy(&xi, in.data() +   0, sizeof(xi));
    std::memcpy(&yi, in.data() +   4, sizeof(yi));
    std::memcpy(&zi, in.data() +   8, sizeof(zi));

    // Apply to existing GameObject
    m_gameObject->transform.translation.x = float(xi);
    m_gameObject->transform.translation.y = float(yi);
    m_gameObject->transform.translation.z = float(zi);

    // 3) Clear existing blocks: fill with air
    fill(0, 0, 0, CHUNK_SIZE - 1, CHUNK_SIZE - 1, CHUNK_SIZE - 1, BlockType::AIR);

    // 4) RLE-decode blocks into the vector
    size_t idx = HEADER;
    size_t write = 0;
    while (idx + 2 <= in.size() && write < m_blocks.size()) {
        uint8_t t     = static_cast<uint8_t>(in[idx]);
        uint8_t count = static_cast<uint8_t>(in[idx+1]);
        idx += 2;
        for (uint8_t c = 0; c < count && write < m_blocks.size(); ++c) {
            m_blocks[write++].type = BlockType(t);
        }
    }
}

bool Chunk::allNeighborsLoaded() const {
    for (const auto& neighbor : m_neighbors) {
        if (!neighbor) return false;
    }
    return true;
}

void Chunk::clearMesh() {
    m_vertices.clear();
    m_indices.clear();
    flags &= ~ChunkFlags::MESH_GENERATED;
}

ChunkCoord Chunk::getChunkCoord() const {
    return { static_cast<int>(m_gameObject->transform.translation.x / CHUNK_SIZE),
             static_cast<int>(m_gameObject->transform.translation.y / CHUNK_SIZE),
             static_cast<int>(m_gameObject->transform.translation.z / CHUNK_SIZE) };
}

void Chunk::setMeshGenerated(bool generated) {
    if (generated) {
        flags |= ChunkFlags::MESH_GENERATED;
    } else {
        flags &= ~ChunkFlags::MESH_GENERATED;
    }
}
void Chunk::setUpToDate(bool upToDate) {
    if (upToDate) {
        flags |= ChunkFlags::UP_TO_DATE;
    } else {
        flags &= ~ChunkFlags::UP_TO_DATE;
    }
}

} // namespace vkengine