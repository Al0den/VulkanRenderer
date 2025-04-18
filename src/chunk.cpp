#include "chunk.hpp"
#include "game_object.hpp"
#include "perlin_noise.hpp"

namespace vkengine {

Chunk::Chunk(Device &deviceRef, std::shared_ptr<GameObject> gameObject) 
    : device{deviceRef}, m_gameObject(gameObject) {
    initialize();
}

Chunk::~Chunk() {
    // Cleanup if needed
}

void Chunk::initialize() {
    // Initialize all blocks as AIR by default
    for (auto& block : m_blocks) {
        block.type = BlockType::AIR;
    }
}

void Chunk::generateTerrain() {
    // Lock the chunk to prevent concurrent modifications
    { 
        std::lock_guard<std::mutex> lock(m_chunkMutex);
        // Get the chunk's Y coordinate from the game object's position
        float chunkYPos = m_gameObject->transform.translation.y;
        int chunkY = static_cast<int>(chunkYPos / CHUNK_SIZE);
        
        // Different terrain generation based on chunk's Y position
        if (chunkY < 0) {
            // For chunks above ground level (y > 0), just fill with air
            // Air is the default, so we don't need to do anything
            
        } else if (chunkY > 0) {
            // For chunks below ground (y < 0), fill with solid stone
            fill(0, 0, 0, CHUNK_SIZE - 1, CHUNK_SIZE - 1, CHUNK_SIZE - 1, BlockType::STONE);
            
        } else {
            // For the ground level chunk (y = 0), use the original terrain generation algorithm
            
            // Generate a simple heightmap
            std::array<int, CHUNK_SIZE * CHUNK_SIZE> heightMap;
            
            // Create a Perlin noise generator with a fixed seed for consistent terrain
            static PerlinNoise perlin(42);
            
            // Scale factors for noise generation
            const double scale = 0.1;      // Controls the "zoom" level of the terrain
            const int octaves = 4;         // Number of noise layers to stack
            const double persistence = 0.5; // How much each octave contributes
            const double heightScale = 3.0; // Amplifies the terrain height variation
            const double baseHeight = CHUNK_SIZE / 2.0f; // Base height level
            
            // Get chunk world position for continuous terrain across chunks
            float chunkXPos = m_gameObject->transform.translation.x;
            float chunkZPos = m_gameObject->transform.translation.z;
            int chunkX = static_cast<int>(chunkXPos / CHUNK_SIZE);
            int chunkZ = static_cast<int>(chunkZPos / CHUNK_SIZE);
            
            // Generate height values using Perlin noise
            for (int x = 0; x < CHUNK_SIZE; x++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    // Calculate world coordinates for noise input
                    double worldX = (chunkX * CHUNK_SIZE + x) * scale;
                    double worldZ = (chunkZ * CHUNK_SIZE + z) * scale;
                    
                    // Generate noise value using multiple octaves for more natural terrain
                    double noiseValue = perlin.octaveNoise(worldX, worldZ, octaves, persistence);
                    
                    // Convert the noise from [-1,1] to [0,1] range
                    noiseValue = (noiseValue + 1.0) * 0.5;
                    
                    // Apply height scaling and add base height
                    float heightValue = baseHeight + static_cast<float>(noiseValue * heightScale);
                    
                    // Add some mountains in specific areas (using additional noise function)
                    double mountainNoise = perlin.noise(worldX * 0.5, worldZ * 0.5);
                    if (mountainNoise > 0.3) {
                        // Add higher peaks where mountain noise is strong
                        heightValue += static_cast<float>((mountainNoise - 0.3) * 12.0);
                    }
                    
                    // Clamp height to valid range (0 to CHUNK_SIZE-1)
                    int height = static_cast<int>(std::min(std::max(heightValue, 0.0f), static_cast<float>(CHUNK_SIZE - 1)));
                    
                    // Store in heightmap
                    heightMap[x + z * CHUNK_SIZE] = height;
                }
            }
            
            // Use the heightmap to set blocks
            for (int x = 0; x < CHUNK_SIZE; x++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    int height = heightMap[x + z * CHUNK_SIZE];
                    
                    // Invert Y coordinates here: 
                    // CHUNK_SIZE - 1 is the highest Y value in the chunk
                    // We'll map the heights so that higher terrain is at lower Y values (negative Y in rendering)
                    
                    // Top layer is grass (at the lowest Y coordinate)
                    setBlock(x, CHUNK_SIZE - 1 - height, z, BlockType::GRASS);
                    
                    // Add dirt layer
                    for (int y = CHUNK_SIZE - 1 - height + 1; y < CHUNK_SIZE - 1 - height + 4; y++) {
                        if (y < CHUNK_SIZE) {
                            setBlock(x, y, z, BlockType::DIRT);
                        }
                    }
                    
                    // Fill above surface with stone
                    for (int y = CHUNK_SIZE - 1 - height + 4; y < CHUNK_SIZE; y++) {
                        if (y < CHUNK_SIZE) {
                            setBlock(x, y, z, BlockType::STONE);
                        }
                    }
                    
                    // Add water in higher areas (which are now at lower Y values)
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
        
        // Mark as modified to ensure the mesh gets regenerated
        m_modified = true;
    }
}

void Chunk::fill(int x1, int y1, int z1, int x2, int y2, int z2, BlockType blockType) {
    // Ensure coordinates are in ascending order
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);
    if (z1 > z2) std::swap(z1, z2);
    
    // Clamp coordinates to chunk bounds
    x1 = std::max(0, std::min(x1, CHUNK_SIZE - 1));
    y1 = std::max(0, std::min(y1, CHUNK_SIZE - 1));
    z1 = std::max(0, std::min(z1, CHUNK_SIZE - 1));
    x2 = std::max(0, std::min(x2, CHUNK_SIZE - 1));
    y2 = std::max(0, std::min(y2, CHUNK_SIZE - 1));
    z2 = std::max(0, std::min(z2, CHUNK_SIZE - 1));
    
    // Fill the region with the specified block type
    for (int x = x1; x <= x2; x++) {
        for (int y = y1; y <= y2; y++) {
            for (int z = z1; z <= z2; z++) {
                setBlock(x, y, z, blockType);
            }
        }
    }
    
    // Mark as modified to ensure the mesh gets regenerated
    m_modified = true;
}

void Chunk::setBlock(int x, int y, int z, BlockType blockType) {
    if (isInBounds(x, y, z)) {
        int index = coordsToIndex(x, y, z);
        
        // Only mark as modified if the block type actually changes
        if (m_blocks[index].type != blockType) {
            m_blocks[index].type = blockType;
            m_modified = true;  // Set modified flag when a block is changed
        }
    }
}

Block Chunk::getBlock(int x, int y, int z) const {
    if (isInBounds(x, y, z)) {
        int index = coordsToIndex(x, y, z);
        return m_blocks[index];
    }
    
    // Return air for out-of-bounds coordinates
    return Block(BlockType::AIR);
}

bool Chunk::isInBounds(int x, int y, int z) const {
    return (x >= 0 && x < CHUNK_SIZE &&
            y >= 0 && y < CHUNK_SIZE &&
            z >= 0 && z < CHUNK_SIZE);
}

int Chunk::coordsToIndex(int x, int y, int z) const {
    // Convert 3D coordinates to a 1D array index
    // Using the formula: index = x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE
    return x + (y * CHUNK_SIZE) + (z * CHUNK_SIZE * CHUNK_SIZE);
}

void Chunk::generateMesh() {
    {
        std::lock_guard<std::mutex> lock(m_chunkMutex);
        // Check if the chunk is modified before generating the mesh 

        // Skip mesh generation if the chunk hasn't been modified
        if (!m_modified) {
            return;
        }
        
        // Clear previous mesh data
        m_readyToRender = false;
        m_vertices.clear();
        m_indices.clear();
        
        // For each non-AIR block, check if it has any AIR neighbors (which would make it visible)
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    Block block = getBlock(x, y, z);
                    
                    // Skip air blocks (nothing to render)
                    if (block.type == BlockType::AIR) {
                        continue;
                    }
                    
                    // Check each of the six faces to see if they are exposed to air
                    // For each exposed face, add vertices and indices to the mesh
                    
                    // Check TOP face (y+)
                    if (y < CHUNK_SIZE - 1) {
                        if (getBlock(x, y + 1, z).type == BlockType::AIR) {
                            addBlockFace(x, y, z, block.type, Direction::TOP);
                        }
                    } else {
                        // Block is at the chunk boundary, always render the face
                        addBlockFace(x, y, z, block.type, Direction::TOP);
                    }
                    
                    // Check BOTTOM face (y-)
                    if (y > 0) {
                        if (getBlock(x, y - 1, z).type == BlockType::AIR) {
                            addBlockFace(x, y, z, block.type, Direction::BOTTOM);
                        }
                    } else {
                        addBlockFace(x, y, z, block.type, Direction::BOTTOM);
                    }
                    
                    // Check FRONT face (z-)
                    if (z > 0) {
                        if (getBlock(x, y, z - 1).type == BlockType::AIR) {
                            addBlockFace(x, y, z, block.type, Direction::FRONT);
                        }
                    } else {
                        addBlockFace(x, y, z, block.type, Direction::FRONT);
                    }
                    
                    // Check BACK face (z+)
                    if (z < CHUNK_SIZE - 1) {
                        if (getBlock(x, y, z + 1).type == BlockType::AIR) {
                            addBlockFace(x, y, z, block.type, Direction::BACK);
                        }
                    } else {
                        addBlockFace(x, y, z, block.type, Direction::BACK);
                    }
                    
                    // Check LEFT face (x-)
                    if (x > 0) {
                        if (getBlock(x - 1, y, z).type == BlockType::AIR) {
                            addBlockFace(x, y, z, block.type, Direction::LEFT);
                        }
                    } else {
                        addBlockFace(x, y, z, block.type, Direction::LEFT);
                    }
                    
                    // Check RIGHT face (x+)
                    if (x < CHUNK_SIZE - 1) {
                        if (getBlock(x + 1, y, z).type == BlockType::AIR) {
                            addBlockFace(x, y, z, block.type, Direction::RIGHT);
                        }
                    } else {
                        addBlockFace(x, y, z, block.type, Direction::RIGHT);
                    }
                }
            }
        }
        
        // Mark as not modified since we've just updated the mesh
        m_modified = false;
    }
}

void Chunk::updateGameObject() {
    // Update the game object's model with the current vertices and indices
    if (m_gameObject && !m_vertices.empty() && !m_indices.empty()) {
        // Create a new model or update existing model with the mesh data
        Model::Builder builder{};
        builder.vertices = m_vertices;
        builder.indices = m_indices;
        m_gameObject->model = std::make_shared<Model>(device, builder);
    }
    m_readyToRender = true; // Mark as ready to render
}

void Chunk::addBlockFace(int x, int y, int z, BlockType blockType, Direction direction) {
    // Get the current number of vertices to calculate indices offsets
    uint32_t vertexOffset = static_cast<uint32_t>(m_vertices.size());
    
    // Define the color based on block type
    glm::vec3 color;
    switch (blockType) {
        case BlockType::GRASS:
            color = {0.0f, 0.8f, 0.0f}; // Green
            break;
        case BlockType::DIRT:
            color = {0.6f, 0.3f, 0.0f}; // Brown
            break;
        case BlockType::STONE:
            color = {0.5f, 0.5f, 0.5f}; // Gray
            break;
        case BlockType::SAND:
            color = {0.9f, 0.8f, 0.6f}; // Sandy
            break;
        case BlockType::WATER:
            color = {0.0f, 0.0f, 0.8f}; // Blue
            break;
        case BlockType::WOOD:
            color = {0.4f, 0.2f, 0.0f}; // Dark brown
            break;
        case BlockType::LEAVES:
            color = {0.0f, 0.5f, 0.0f}; // Dark green
            break;
        default:
            color = {1.0f, 1.0f, 1.0f}; // White
    }
    
    // Base coordinates (bottom-left-front corner of the block)
    float bx = static_cast<float>(x);
    float by = static_cast<float>(y);
    float bz = static_cast<float>(z);
    
    // Add vertices and indices based on the direction
    // Each face consists of two triangles (6 indices) and 4 vertices
    
    // Define vertices for the specific face
    std::vector<Model::Vertex> faceVertices;
    
    switch (direction) {
        case Direction::TOP: {
            // Top face (y+)
            faceVertices = {
                // Flipped winding order to correct orientation
                {{bx, by + 1.0f, bz + 1.0f}, color, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},  // Back-left
                {{bx + 1.0f, by + 1.0f, bz + 1.0f}, color, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},  // Back-right
                {{bx + 1.0f, by + 1.0f, bz}, color, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},  // Front-right
                {{bx, by + 1.0f, bz}, color, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}  // Front-left
            };
            break;
        }
        case Direction::BOTTOM: {
            // Bottom face (y-)
            faceVertices = {
                // Flipped winding order to correct orientation
                {{bx, by, bz}, color, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
                {{bx + 1.0f, by, bz}, color, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
                {{bx + 1.0f, by, bz + 1.0f}, color, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
                {{bx, by, bz + 1.0f}, color, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}}
            };
            break;
        }
        case Direction::FRONT: {
            // Front face (z-)
            faceVertices = {
                // Flipped winding order to correct orientation
                {{bx, by + 1.0f, bz}, color, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
                {{bx + 1.0f, by + 1.0f, bz}, color, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
                {{bx + 1.0f, by, bz}, color, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
                {{bx, by, bz}, color, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}}
            };
            break;
        }
        case Direction::BACK: {
            // Back face (z+)
            faceVertices = {
                // Flipped winding order to correct orientation
                {{bx, by + 1.0f, bz + 1.0f}, color, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                {{bx + 1.0f, by + 1.0f, bz + 1.0f}, color, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
                {{bx + 1.0f, by, bz + 1.0f}, color, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
                {{bx, by, bz + 1.0f}, color, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}
            };
            break;
        }
        case Direction::LEFT: {
            // Left face (x-)
            faceVertices = {
                // Flipped winding order to correct orientation
                {{bx, by + 1.0f, bz + 1.0f}, color, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
                {{bx, by + 1.0f, bz}, color, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
                {{bx, by, bz}, color, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
                {{bx, by, bz + 1.0f}, color, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}
            };
            break;
        }
        case Direction::RIGHT: {
            // Right face (x+)
            faceVertices = {
                // Flipped winding order to correct orientation
                {{bx + 1.0f, by + 1.0f, bz}, color, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
                {{bx + 1.0f, by + 1.0f, bz + 1.0f}, color, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
                {{bx + 1.0f, by, bz + 1.0f}, color, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
                {{bx + 1.0f, by, bz}, color, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}
            };
            break;
        }
    }
    
    // Add the vertices to the mesh
    m_vertices.insert(m_vertices.end(), faceVertices.begin(), faceVertices.end());
    
    // Add indices for the face (two triangles)
    m_indices.push_back(vertexOffset);
    m_indices.push_back(vertexOffset + 1);
    m_indices.push_back(vertexOffset + 2);
    m_indices.push_back(vertexOffset);
    m_indices.push_back(vertexOffset + 2);
    m_indices.push_back(vertexOffset + 3);
}

} // namespace vkengine
