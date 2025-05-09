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

void Chunk::generateMesh(const std::unordered_map<ChunkCoord, std::shared_ptr<Chunk>, ChunkCoord::Hash> &chunks) {
    m_vertices.clear();
    m_indices.clear();
    m_vertexCache.clear(); // Clear the vertex cache before regenerating the mesh
    
    glm::vec3 chunkPos = m_gameObject->transform.translation;
    int chunkX = static_cast<int>(chunkPos.x / CHUNK_SIZE);
    int chunkY = static_cast<int>(chunkPos.y / CHUNK_SIZE);
    int chunkZ = static_cast<int>(chunkPos.z / CHUNK_SIZE);
    
    Chunk* neighborXPos = nullptr;
    Chunk* neighborXNeg = nullptr;
    Chunk* neighborYPos = nullptr;
    Chunk* neighborYNeg = nullptr;
    Chunk* neighborZPos = nullptr;
    Chunk* neighborZNeg = nullptr;
    
    ChunkCoord coordXPos{chunkX + 1, chunkY, chunkZ};
    ChunkCoord coordXNeg{chunkX - 1, chunkY, chunkZ};
    ChunkCoord coordYPos{chunkX, chunkY + 1, chunkZ};
    ChunkCoord coordYNeg{chunkX, chunkY - 1, chunkZ};
    ChunkCoord coordZPos{chunkX, chunkY, chunkZ + 1};
    ChunkCoord coordZNeg{chunkX, chunkY, chunkZ - 1};
    
    auto itXPos = chunks.find(coordXPos);
    if (itXPos != chunks.end() && itXPos->second->defaultTerrainGenerated()) {
        neighborXPos = itXPos->second.get();
    }
    
    auto itXNeg = chunks.find(coordXNeg);
    if (itXNeg != chunks.end() && itXNeg->second->defaultTerrainGenerated()) {
        neighborXNeg = itXNeg->second.get();
    }
    
    auto itYPos = chunks.find(coordYPos);
    if (itYPos != chunks.end() && itYPos->second->defaultTerrainGenerated()) {
        neighborYPos = itYPos->second.get();
    }
    
    auto itYNeg = chunks.find(coordYNeg);
    if (itYNeg != chunks.end() && itYNeg->second->defaultTerrainGenerated()) {
        neighborYNeg = itYNeg->second.get();
    }
    
    auto itZPos = chunks.find(coordZPos);
    if (itZPos != chunks.end() && itZPos->second->defaultTerrainGenerated()) {
        neighborZPos = itZPos->second.get();
    }
    
    auto itZNeg = chunks.find(coordZNeg);
    if (itZNeg != chunks.end() && itZNeg->second->defaultTerrainGenerated()) {
        neighborZNeg = itZNeg->second.get();
    }
    

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                Block block = getBlock(x, y, z);
                
                if (block.type == BlockType::AIR) {
                    continue;
                }
                
                if (y < CHUNK_SIZE - 1) {
                    if (getBlock(x, y + 1, z).type == BlockType::AIR) {
                        addBlockFace(x, y, z, block.type, Direction::TOP);
                    }
                } else if (neighborYPos) {
                    if (neighborYPos->getBlock(x, 0, z).type == BlockType::AIR) {
                        addBlockFace(x, y, z, block.type, Direction::TOP);
                    }
                } else {
                    addBlockFace(x, y, z, block.type, Direction::TOP);
                }
                
                if (y > 0) {
                    if (getBlock(x, y - 1, z).type == BlockType::AIR) {
                        addBlockFace(x, y, z, block.type, Direction::BOTTOM);
                    }
                } else if (neighborYNeg) {
                    if (neighborYNeg->getBlock(x, CHUNK_SIZE - 1, z).type == BlockType::AIR) {
                        addBlockFace(x, y, z, block.type, Direction::BOTTOM);
                    }
                } else {
                    addBlockFace(x, y, z, block.type, Direction::BOTTOM);
                }
                
                if (z > 0) {
                    if (getBlock(x, y, z - 1).type == BlockType::AIR) {
                        addBlockFace(x, y, z, block.type, Direction::FRONT);
                    }
                } else if (neighborZNeg) {
                    if (neighborZNeg->getBlock(x, y, CHUNK_SIZE - 1).type == BlockType::AIR) {
                        addBlockFace(x, y, z, block.type, Direction::FRONT);
                    }
                } else {
                    addBlockFace(x, y, z, block.type, Direction::FRONT);
                }
                
                if (z < CHUNK_SIZE - 1) {
                    if (getBlock(x, y, z + 1).type == BlockType::AIR) {
                        addBlockFace(x, y, z, block.type, Direction::BACK);
                    }
                } else if (neighborZPos) {
                    if (neighborZPos->getBlock(x, y, 0).type == BlockType::AIR) {
                        addBlockFace(x, y, z, block.type, Direction::BACK);
                    }
                } else {
                    addBlockFace(x, y, z, block.type, Direction::BACK);
                }

                if (x > 0) {
                    if (getBlock(x - 1, y, z).type == BlockType::AIR) {
                        addBlockFace(x, y, z, block.type, Direction::LEFT);
                    }
                } else if (neighborXNeg) {
                    if (neighborXNeg->getBlock(CHUNK_SIZE - 1, y, z).type == BlockType::AIR) {
                        addBlockFace(x, y, z, block.type, Direction::LEFT);
                    }
                } else {
                    addBlockFace(x, y, z, block.type, Direction::LEFT);
                }
                
                if (x < CHUNK_SIZE - 1) {
                    if (getBlock(x + 1, y, z).type == BlockType::AIR) {
                        addBlockFace(x, y, z, block.type, Direction::RIGHT);
                    }
                } else if (neighborXPos) {
                    if (neighborXPos->getBlock(0, y, z).type == BlockType::AIR) {
                        addBlockFace(x, y, z, block.type, Direction::RIGHT);
                    }
                } else {
                    addBlockFace(x, y, z, block.type, Direction::RIGHT);
                }
            }
        }
    }

    m_meshGenerated = true;
    m_upToDate = false;
}

void Chunk::generateGreedyMesh(const std::unordered_map<ChunkCoord, std::shared_ptr<Chunk>, ChunkCoord::Hash> &chunks) {
    m_vertices.clear();
    m_indices.clear();
    
    // Get chunk position and calculate chunk coordinates
    glm::vec3 chunkPos = m_gameObject->transform.translation;
    int chunkX = static_cast<int>(chunkPos.x / CHUNK_SIZE);
    int chunkY = static_cast<int>(chunkPos.y / CHUNK_SIZE);
    int chunkZ = static_cast<int>(chunkPos.z / CHUNK_SIZE);
    
    // Find neighboring chunks
    Chunk* neighborXPos = nullptr;
    Chunk* neighborXNeg = nullptr;
    Chunk* neighborYPos = nullptr;
    Chunk* neighborYNeg = nullptr;
    Chunk* neighborZPos = nullptr;
    Chunk* neighborZNeg = nullptr;
    
    // Define chunk coordinates for neighbors
    ChunkCoord coordXPos{chunkX + 1, chunkY, chunkZ};
    ChunkCoord coordXNeg{chunkX - 1, chunkY, chunkZ};
    ChunkCoord coordYPos{chunkX, chunkY + 1, chunkZ};
    ChunkCoord coordYNeg{chunkX, chunkY - 1, chunkZ};
    ChunkCoord coordZPos{chunkX, chunkY, chunkZ + 1};
    ChunkCoord coordZNeg{chunkX, chunkY, chunkZ - 1};
    
    // Look up neighbors from chunks map
    auto itXPos = chunks.find(coordXPos);
    auto itXNeg = chunks.find(coordXNeg);
    auto itYPos = chunks.find(coordYPos);
    auto itYNeg = chunks.find(coordYNeg);
    auto itZPos = chunks.find(coordZPos);
    auto itZNeg = chunks.find(coordZNeg);

    if (itXPos != chunks.end() && itXPos->second->defaultTerrainGenerated()) {
        neighborXPos = itXPos->second.get();
    }
    
    
    if (itXNeg != chunks.end() && itXNeg->second->defaultTerrainGenerated()) {
        neighborXNeg = itXNeg->second.get();
    }
    
    if (itYPos != chunks.end() && itYPos->second->defaultTerrainGenerated()) {
        neighborYPos = itYPos->second.get();
    }
    
    if (itYNeg != chunks.end() && itYNeg->second->defaultTerrainGenerated()) {
        neighborYNeg = itYNeg->second.get();
    }
    
    if (itZPos != chunks.end() && itZPos->second->defaultTerrainGenerated()) {
        neighborZPos = itZPos->second.get();
    }   
    
    if (itZNeg != chunks.end() && itZNeg->second->defaultTerrainGenerated()) {
        neighborZNeg = itZNeg->second.get();
    }
    
    std::array<std::array<bool, CHUNK_SIZE * CHUNK_SIZE>, 6> faceMasks;
    std::array<std::array<BlockType, CHUNK_SIZE * CHUNK_SIZE>, 6> faceTypes;
    
    // Direction indices for more concise code
    const int TOP = 0, BOTTOM = 1, FRONT = 2, BACK = 3, LEFT = 4, RIGHT = 5;
    
    // Generate mask for RIGHT faces (X+)
    for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                Block block = getBlock(x, y, z);
                bool shouldRender = false;
                
                if (block.type != BlockType::AIR) {
                    // Right face check for greedy meshing
                    if (x < CHUNK_SIZE - 1) {
                        shouldRender = getBlock(x + 1, y, z).type == BlockType::AIR;
                    } else if (neighborXPos) {
                        shouldRender = neighborXPos->getBlock(0, y, z).type == BlockType::AIR;
                    } else {
                        shouldRender = true; // Render if at chunk boundary with no neighbor
                    }
                    
                    if (shouldRender) {
                        faceMasks[RIGHT][y + z * CHUNK_SIZE] = true;
                        faceTypes[RIGHT][y + z * CHUNK_SIZE] = block.type;
                    }
                    
                    // Left face check for greedy meshing
                    if (x > 0) {
                        shouldRender = getBlock(x - 1, y, z).type == BlockType::AIR;
                    } else if (neighborXNeg) {
                        shouldRender = neighborXNeg->getBlock(CHUNK_SIZE - 1, y, z).type == BlockType::AIR;
                    } else {
                        shouldRender = true; // Render if at chunk boundary with no neighbor
                    }
                    
                    if (shouldRender) {
                        faceMasks[LEFT][y + z * CHUNK_SIZE] = true;
                        faceTypes[LEFT][y + z * CHUNK_SIZE] = block.type;
                    }
                    
                    // Back face check for greedy meshing
                    if (z < CHUNK_SIZE - 1) {
                        shouldRender = getBlock(x, y, z + 1).type == BlockType::AIR;
                    } else if (neighborZPos) {
                        shouldRender = neighborZPos->getBlock(x, y, 0).type == BlockType::AIR;
                    } else {
                        shouldRender = true; // Render if at chunk boundary with no neighbor
                    }
                    
                    if (shouldRender) {
                        faceMasks[BACK][x + y * CHUNK_SIZE] = true;
                        faceTypes[BACK][x + y * CHUNK_SIZE] = block.type;
                    }

                    // Front face check for greedy meshing
                    if (z > 0) {
                        shouldRender = getBlock(x, y, z - 1).type == BlockType::AIR;
                    } else if (neighborZNeg) {
                        shouldRender = neighborZNeg->getBlock(x, y, CHUNK_SIZE - 1).type == BlockType::AIR;
                    } else {
                        shouldRender = true; // Render if at chunk boundary with no neighbor
                    }
                    
                    if (shouldRender) {
                        faceMasks[FRONT][x + y * CHUNK_SIZE] = true;
                        faceTypes[FRONT][x + y * CHUNK_SIZE] = block.type;
                    }

                    // Bottom face check for greedy meshing
                    if (y > 0) {
                        shouldRender = getBlock(x, y - 1, z).type == BlockType::AIR;
                    } else if (neighborYNeg) {
                        shouldRender = neighborYNeg->getBlock(x, CHUNK_SIZE - 1, z).type == BlockType::AIR;
                    } else {
                        shouldRender = true; // Render if at chunk boundary with no neighbor
                    }
                    
                    if (shouldRender) {
                        faceMasks[BOTTOM][x + z * CHUNK_SIZE] = true;
                        faceTypes[BOTTOM][x + z * CHUNK_SIZE] = block.type;
                    }

                    if (y < CHUNK_SIZE - 1) {
                        shouldRender = getBlock(x, y + 1, z).type == BlockType::AIR;
                    } else if (neighborYPos) {
                        shouldRender = neighborYPos->getBlock(x, 0, z).type == BlockType::AIR;
                    } else {
                        shouldRender = true; // Render if at chunk boundary with no neighbor
                    }
                    
                    if (shouldRender) {
                        faceMasks[TOP][x + z * CHUNK_SIZE] = true;
                        faceTypes[TOP][x + z * CHUNK_SIZE] = block.type;
                    }
                }
            }
        }
    }
    
    // Greedy meshing algorithm for each direction
    // Helper function to get mask value with bounds checking
    auto getMask = [&](const std::array<bool, CHUNK_SIZE * CHUNK_SIZE>& mask, int x, int y) -> bool {
        if (x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_SIZE) {
            return mask[x + y * CHUNK_SIZE];
        }
        return false;
    };
    
    auto getType = [&](const std::array<BlockType, CHUNK_SIZE * CHUNK_SIZE>& types, int x, int y) -> BlockType {
        if (x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_SIZE) {
            return types[x + y * CHUNK_SIZE];
        }
        return BlockType::AIR;
    };
    
    // Process each direction
    for (int dir = 0; dir < 6; dir++) {
        // Create a visited mask to track which faces we've already processed
        std::array<bool, CHUNK_SIZE * CHUNK_SIZE> visited{};
        
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                if (!getMask(faceMasks[dir], x, y) || visited[x + y * CHUNK_SIZE]) {
                    // Skip if no face to render or already processed
                    continue;
                }
                
                BlockType blockType = getType(faceTypes[dir], x, y);
                
                // Find the width of the rectangle (how far we can go in x direction)
                int width = 1;
                while (x + width < CHUNK_SIZE && 
                       getMask(faceMasks[dir], x + width, y) && 
                       !visited[x + width + y * CHUNK_SIZE] &&
                       getType(faceTypes[dir], x + width, y) == blockType) {
                    width++;
                }
                
                // Find the height of the rectangle (how far we can go in y direction)
                int height = 1;
                bool canExtendHeight = true;
                
                while (canExtendHeight && y + height < CHUNK_SIZE) {
                    // Check if we can extend the entire width
                    for (int dx = 0; dx < width; dx++) {
                        if (!getMask(faceMasks[dir], x + dx, y + height) || 
                            visited[x + dx + (y + height) * CHUNK_SIZE] ||
                            getType(faceTypes[dir], x + dx, y + height) != blockType) {
                            canExtendHeight = false;
                            break;
                        }
                    }
                    
                    if (canExtendHeight) {
                        height++;
                    }
                }
                
                // Mark all cells in the rectangle as visited
                for (int dy = 0; dy < height; dy++) {
                    for (int dx = 0; dx < width; dx++) {
                        visited[x + dx + (y + dy) * CHUNK_SIZE] = true;
                    }
                }
                
                // Now we have a rectangle from (x,y) with dimensions (width,height)
                // We need to create a face for this rectangle
                // The coordinates to use depend on which direction we're processing
                
                float bx, by, bz; // base coordinates
                float w, h;       // width and height of the face
                glm::vec3 normal;
                Direction faceDir;
                
                switch (dir) {
                    case 0: // TOP (Y+)
                        // For TOP faces, the y coordinate is fixed, x and z vary
                        // We need to find the y coordinate of the highest block in this column
                        for (int dy = CHUNK_SIZE - 1; dy >= 0; dy--) {
                            if (getBlock(x, dy, y).type == blockType) {
                                bx = static_cast<float>(x);
                                by = static_cast<float>(dy + 1); // TOP face is at y+1
                                bz = static_cast<float>(y);
                                w = static_cast<float>(width);
                                h = static_cast<float>(height);
                                normal = {0.0f, 1.0f, 0.0f};
                                faceDir = Direction::TOP;
                                addGreedyFace(bx, by, bz, w, h, blockType, faceDir, normal, true);
                                break;
                            }
                        }
                        break;
                        
                    case 1: // BOTTOM (Y-)
                        // For BOTTOM faces, the y coordinate is fixed, x and z vary
                        for (int dy = 0; dy < CHUNK_SIZE; dy++) {
                            if (getBlock(x, dy, y).type == blockType) {
                                bx = static_cast<float>(x);
                                by = static_cast<float>(dy); // BOTTOM face is at y
                                bz = static_cast<float>(y);
                                w = static_cast<float>(width);
                                h = static_cast<float>(height);
                                normal = {0.0f, -1.0f, 0.0f};
                                faceDir = Direction::BOTTOM;
                                addGreedyFace(bx, by, bz, w, h, blockType, faceDir, normal, false);
                                break;
                            }
                        }
                        break;
                        
                    case 2: // FRONT (Z-)
                        // For FRONT faces, the z coordinate is fixed (z=0), x and y vary
                        // In our mask, 'x' is the x-coordinate and 'y' is the y-coordinate
                        bx = static_cast<float>(x);
                        by = static_cast<float>(y); 
                        bz = static_cast<float>(0); // FRONT is at z=0
                        w = static_cast<float>(width);
                        h = static_cast<float>(height);
                        normal = {0.0f, 0.0f, -1.0f};
                        faceDir = Direction::FRONT;
                        addGreedyFace(bx, by, bz, w, h, blockType, faceDir, normal, false);
                        break;
                        
                    case 3: // BACK (Z+)
                        // For BACK faces, the z coordinate is fixed (z=CHUNK_SIZE), x and y vary
                        // In our mask, 'x' is the x-coordinate and 'y' is the y-coordinate
                        bx = static_cast<float>(x);
                        by = static_cast<float>(y);
                        bz = static_cast<float>(CHUNK_SIZE); // BACK face is at z=CHUNK_SIZE
                        w = static_cast<float>(width);
                        h = static_cast<float>(height);
                        normal = {0.0f, 0.0f, 1.0f};
                        faceDir = Direction::BACK;
                        addGreedyFace(bx, by, bz, w, h, blockType, faceDir, normal, true);
                        break;
                        
                    case 4: // LEFT (X-)
                        // For LEFT faces, the x coordinate is fixed (x=0), y and z vary
                        // In our mask, 'x' is the y-coordinate and 'y' is the z-coordinate
                        bx = static_cast<float>(0); // LEFT face is at x=0
                        by = static_cast<float>(x); // 'x' in our loop is actually the y-coordinate
                        bz = static_cast<float>(y); // 'y' in our loop is actually the z-coordinate
                        w = static_cast<float>(width);
                        h = static_cast<float>(height);
                        normal = {-1.0f, 0.0f, 0.0f};
                        faceDir = Direction::LEFT;
                        addGreedyFace(bx, by, bz, w, h, blockType, faceDir, normal, false);
                        break;
                        
                    case 5: // RIGHT (X+)
                        // For RIGHT faces, the x coordinate is fixed (x=CHUNK_SIZE), y and z vary
                        // In our mask, 'x' is the y-coordinate and 'y' is the z-coordinate
                        bx = static_cast<float>(CHUNK_SIZE); // RIGHT face is at x=CHUNK_SIZE
                        by = static_cast<float>(x); // 'x' in our loop is actually the y-coordinate
                        bz = static_cast<float>(y); // 'y' in our loop is actually the z-coordinate
                        w = static_cast<float>(width);
                        h = static_cast<float>(height);
                        normal = {1.0f, 0.0f, 0.0f};
                        faceDir = Direction::RIGHT;
                        addGreedyFace(bx, by, bz, w, h, blockType, faceDir, normal, true);
                        break;
                        break;
                }
            }
        }
    }
    
    m_meshGenerated = true;
    m_upToDate = false;
}

// Helper function for greedy meshing to add a merged face
void Chunk::addGreedyFace(float x, float y, float z, float width, float height, 
                          BlockType blockType, Direction direction, 
                          const glm::vec3& normal, bool flipWinding) {
    uint32_t vertexOffset = static_cast<uint32_t>(m_vertices.size());
    
    // Determine the color based on block type
    glm::vec3 color;
    switch (blockType) {
        case BlockType::GRASS:
            color = {0.0f, 0.8f, 0.0f};
            break;
        case BlockType::DIRT:
            color = {0.6f, 0.3f, 0.0f};
            break;
        case BlockType::STONE:
            color = {0.5f, 0.5f, 0.5f};
            break;
        case BlockType::SAND:
            color = {0.9f, 0.8f, 0.6f};
            break;
        case BlockType::WATER:
            color = {0.0f, 0.0f, 0.8f};
            break;
        case BlockType::WOOD:
            color = {0.4f, 0.2f, 0.0f};
            break;
        case BlockType::LEAVES:
            color = {0.0f, 0.5f, 0.0f};
            break;
        default:
            color = {1.0f, 1.0f, 1.0f};
    }
    
    std::vector<Model::Vertex> faceVertices;
    
    // Create vertices based on direction
    switch (direction) {
        case Direction::TOP:
            // TOP face is in the XZ plane at fixed Y
            faceVertices = {
                {{x, y, z}, color, normal, {0.0f, 0.0f}},
                {{x + width, y, z}, color, normal, {width, 0.0f}},
                {{x + width, y, z + height}, color, normal, {width, height}},
                {{x, y, z + height}, color, normal, {0.0f, height}}
            };
            break;
            
        case Direction::BOTTOM:
            // BOTTOM face is in the XZ plane at fixed Y
            faceVertices = {
                {{x, y, z}, color, normal, {0.0f, 0.0f}},
                {{x + width, y, z}, color, normal, {width, 0.0f}},
                {{x + width, y, z + height}, color, normal, {width, height}},
                {{x, y, z + height}, color, normal, {0.0f, height}}
            };
            break;
            
        case Direction::FRONT:
            // FRONT face is in the XY plane at fixed Z
            faceVertices = {
                {{x, y, z}, color, normal, {0.0f, 0.0f}},
                {{x + width, y, z}, color, normal, {width, 0.0f}},
                {{x + width, y + height, z}, color, normal, {width, height}},
                {{x, y + height, z}, color, normal, {0.0f, height}}
            };
            break;
            
        case Direction::BACK:
            // BACK face is in the XY plane at fixed Z
            faceVertices = {
                {{x, y, z}, color, normal, {0.0f, 0.0f}},
                {{x + width, y, z}, color, normal, {width, 0.0f}},
                {{x + width, y + height, z}, color, normal, {width, height}},
                {{x, y + height, z}, color, normal, {0.0f, height}}
            };
            break;
            
        case Direction::LEFT:
            // LEFT face is in the YZ plane at fixed X
            faceVertices = {
                {{x, y, z}, color, normal, {0.0f, 0.0f}},
                {{x, y, z + width}, color, normal, {width, 0.0f}},
                {{x, y + height, z + width}, color, normal, {width, height}},
                {{x, y + height, z}, color, normal, {0.0f, height}}
            };
            break;
            
        case Direction::RIGHT:
            // RIGHT face is in the YZ plane at fixed X
            faceVertices = {
                {{x, y, z}, color, normal, {0.0f, 0.0f}},
                {{x, y, z + width}, color, normal, {width, 0.0f}},
                {{x, y + height, z + width}, color, normal, {width, height}},
                {{x, y + height, z}, color, normal, {0.0f, height}}
            };
            break;
    }
    
    m_vertices.insert(m_vertices.end(), faceVertices.begin(), faceVertices.end());
    
    // Create indices for the face
    if (flipWinding) {
        // Counter-clockwise winding
        m_indices.push_back(vertexOffset);
        m_indices.push_back(vertexOffset + 1);
        m_indices.push_back(vertexOffset + 2);
        m_indices.push_back(vertexOffset);
        m_indices.push_back(vertexOffset + 2);
        m_indices.push_back(vertexOffset + 3);
    } else {
        // Clockwise winding
        m_indices.push_back(vertexOffset);
        m_indices.push_back(vertexOffset + 3);
        m_indices.push_back(vertexOffset + 2);
        m_indices.push_back(vertexOffset);
        m_indices.push_back(vertexOffset + 2);
        m_indices.push_back(vertexOffset + 1);
    }
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
                
                setBlock(x, CHUNK_SIZE - 1 - height, z, BlockType::GRASS);
                
                for (int y = CHUNK_SIZE - 1 - height + 1; y < CHUNK_SIZE - 1 - height + 4; y++) {
                    if (y < CHUNK_SIZE) {
                        setBlock(x, y, z, BlockType::DIRT);
                    }
                }
                
                for (int y = CHUNK_SIZE - 1 - height + 4; y < CHUNK_SIZE; y++) {
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

void Chunk::updateGameObject() {
    if (m_gameObject && !m_vertices.empty() && !m_indices.empty()) {
        Model::Builder builder{};
        builder.vertices = m_vertices;
        builder.indices = m_indices;
        m_gameObject->model = std::make_shared<Model>(device, builder);
    } else {
        if (m_gameObject.get() == nullptr) {
            throw std::runtime_error("GameObject is null");
        }
    }

    m_upToDate = true;
}

void Chunk::addBlockFace(int x, int y, int z, BlockType blockType, Direction direction) {
    uint32_t vertexOffset = static_cast<uint32_t>(m_vertices.size());
    
    glm::vec3 color;
    switch (blockType) {
        case BlockType::GRASS:
            color = {0.0f, 0.8f, 0.0f};
            break;
        case BlockType::DIRT:
            color = {0.6f, 0.3f, 0.0f};
            break;
        case BlockType::STONE:
            color = {0.5f, 0.5f, 0.5f};
            break;
        case BlockType::SAND:
            color = {0.9f, 0.8f, 0.6f};
            break;
        case BlockType::WATER:
            color = {0.0f, 0.0f, 0.8f};
            break;
        case BlockType::WOOD:
            color = {0.4f, 0.2f, 0.0f};
            break;
        case BlockType::LEAVES:
            color = {0.0f, 0.5f, 0.0f};
            break;
        default:
            color = {1.0f, 1.0f, 1.0f};
    }
    
    float bx = static_cast<float>(x);
    float by = static_cast<float>(y);
    float bz = static_cast<float>(z);
    
    std::vector<Model::Vertex> faceVertices;
    
    switch (direction) {
        case Direction::TOP: {
            faceVertices = {
                {{bx, by + 1.0f, bz + 1.0f}, color, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
                {{bx + 1.0f, by + 1.0f, bz + 1.0f}, color, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
                {{bx + 1.0f, by + 1.0f, bz}, color, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                {{bx, by + 1.0f, bz}, color, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}
            };
            break;
        }
        case Direction::BOTTOM: {
            faceVertices = {
                {{bx, by, bz}, color, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
                {{bx + 1.0f, by, bz}, color, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
                {{bx + 1.0f, by, bz + 1.0f}, color, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
                {{bx, by, bz + 1.0f}, color, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}}
            };
            break;
        }
        case Direction::FRONT: {
            faceVertices = {
                {{bx, by + 1.0f, bz}, color, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
                {{bx + 1.0f, by + 1.0f, bz}, color, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
                {{bx + 1.0f, by, bz}, color, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
                {{bx, by, bz}, color, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}}
            };
            break;
        }
        case Direction::BACK: {
            faceVertices = {
                {{bx, by + 1.0f, bz + 1.0f}, color, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                {{bx + 1.0f, by + 1.0f, bz + 1.0f}, color, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
                {{bx + 1.0f, by, bz + 1.0f}, color, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
                {{bx, by, bz + 1.0f}, color, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}
            };
            break;
        }
        case Direction::LEFT: {
            faceVertices = {
                {{bx, by + 1.0f, bz + 1.0f}, color, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
                {{bx, by + 1.0f, bz}, color, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
                {{bx, by, bz}, color, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
                {{bx, by, bz + 1.0f}, color, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}
            };
            break;
        }
        case Direction::RIGHT: {
            faceVertices = {
                {{bx + 1.0f, by + 1.0f, bz}, color, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
                {{bx + 1.0f, by + 1.0f, bz + 1.0f}, color, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
                {{bx + 1.0f, by, bz + 1.0f}, color, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
                {{bx + 1.0f, by, bz}, color, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}
            };
            break;
        }
    }
    
    m_vertices.insert(m_vertices.end(), faceVertices.begin(), faceVertices.end());

    m_indices.push_back(vertexOffset);
    m_indices.push_back(vertexOffset + 1);
    m_indices.push_back(vertexOffset + 2);
    m_indices.push_back(vertexOffset);
    m_indices.push_back(vertexOffset + 2);
    m_indices.push_back(vertexOffset + 3);
}

} // namespace vkengine
