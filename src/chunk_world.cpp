#include "chunk.hpp"
#include "game_object.hpp"
#include "perlin_noise.hpp"

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
    float chunkYPos = m_gameObject->transform.translation.y;
    int chunkY = static_cast<int>(chunkYPos / CHUNK_SIZE);
    
    if (chunkY < 0) {
        fill(0, 0, 0, CHUNK_SIZE - 1, CHUNK_SIZE - 1, CHUNK_SIZE - 1, BlockType::AIR);

    } else if (chunkY > 0) {
        fill(0, 0, 0, CHUNK_SIZE - 1, CHUNK_SIZE - 1, CHUNK_SIZE - 1, BlockType::STONE);  
    } else {
        std::array<int, CHUNK_SIZE * CHUNK_SIZE> heightMap;
        
        static PerlinNoise perlin(42);
        
        const double scale = 0.1;
        const int octaves = 4;
        const double persistence = 0.5;
        const double heightScale = 3.0;
        const double baseHeight = CHUNK_SIZE / 2.0f;
        
        float chunkXPos = m_gameObject->transform.translation.x;
        float chunkZPos = m_gameObject->transform.translation.z;
        int chunkX = static_cast<int>(chunkXPos / CHUNK_SIZE);
        int chunkZ = static_cast<int>(chunkZPos / CHUNK_SIZE);
        
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                double worldX = (chunkX * CHUNK_SIZE + x) * scale;
                double worldZ = (chunkZ * CHUNK_SIZE + z) * scale;
                
                double noiseValue = perlin.octaveNoise(worldX, worldZ, octaves, persistence);
                
                noiseValue = (noiseValue + 1.0) * 0.5;
                
                float heightValue = baseHeight + static_cast<float>(noiseValue * heightScale);
                
                double mountainNoise = perlin.noise(worldX * 0.5, worldZ * 0.5);
                if (mountainNoise > 0.3) {
                    heightValue += static_cast<float>((mountainNoise - 0.3) * 12.0);
                }
                
                int height = static_cast<int>(std::min(std::max(heightValue, 0.0f), static_cast<float>(CHUNK_SIZE - 1)));
                
                heightMap[x + z * CHUNK_SIZE] = height;
            }
        }
        
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                int height = heightMap[x + z * CHUNK_SIZE];
                
                // Surface grass
                setBlock(x, CHUNK_SIZE - 1 - height, z, BlockType::GRASS);
                int surfaceY = CHUNK_SIZE - 1 - height;
                // High grass immediately above surface grass
                if (surfaceY + 1 < CHUNK_SIZE) {
                    setBlock(x, surfaceY + 1, z, BlockType::GRASS);
                }
                // Dirt layers above high grass
                for (int y = surfaceY + 2; y < surfaceY + 4; y++) {
                    if (y < CHUNK_SIZE) {
                        setBlock(x, y, z, BlockType::GRASS);
                    }
                }
                // Stone below dirt layers
                for (int y = surfaceY + 4; y < CHUNK_SIZE; y++) {
                    if (y < CHUNK_SIZE) {
                        setBlock(x, y, z, BlockType::STONE);
                    }
                }
                
                if (height > CHUNK_SIZE * 2 / 3) {
                    for (int y = CHUNK_SIZE - 1 - height - 1; y >= CHUNK_SIZE - 1 - CHUNK_SIZE / 3; y--) {
                        if (y >= 0) {
                            setBlock(x, y, z, BlockType::WATER);
                        }
                    }
                }
            }
        }
    }

    m_defaultTerrainGenerated = true;
    m_updated = true;
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
    
    m_meshGenerated = false;
}

void Chunk::setBlock(int x, int y, int z, BlockType blockType) {
    if (isInBounds(x, y, z)) {
        int index = coordsToIndex(x, y, z);
        
        if (m_blocks[index].type != blockType) {
            m_blocks[index].type = blockType;
            m_meshGenerated = false;
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

}