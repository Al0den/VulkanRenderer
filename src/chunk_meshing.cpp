#include "chunk.hpp"
#include "game_object.hpp"
#include "perlin_noise.hpp"

namespace vkengine {


void Chunk::generateMesh() {
    m_vertices.clear();
    m_indices.clear();

    glm::vec3 chunkPos = m_gameObject->transform.translation;
    int chunkX = static_cast<int>(chunkPos.x / CHUNK_SIZE);
    int chunkY = static_cast<int>(chunkPos.y / CHUNK_SIZE);
    int chunkZ = static_cast<int>(chunkPos.z / CHUNK_SIZE);
    
    std::shared_ptr<Chunk> neighborXPos = m_neighbors[0];
    std::shared_ptr<Chunk> neighborXNeg = m_neighbors[1];
    std::shared_ptr<Chunk> neighborYPos = m_neighbors[2];
    std::shared_ptr<Chunk> neighborYNeg = m_neighbors[3];
    std::shared_ptr<Chunk> neighborZPos = m_neighbors[4];
    std::shared_ptr<Chunk> neighborZNeg = m_neighbors[5];

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

    flags |= ChunkFlags::MESH_GENERATED;
    flags &= ~ChunkFlags::UP_TO_DATE;
}

void Chunk::generateGreedyMesh() {
    m_vertices.clear();
    m_indices.clear();

    m_vertices.reserve(CHUNK_SIZE * CHUNK_SIZE * 6);
    m_indices.reserve(CHUNK_SIZE * CHUNK_SIZE * 6);
    
    glm::vec3 chunkPos = m_gameObject->transform.translation;
    int chunkX = static_cast<int>(chunkPos.x / CHUNK_SIZE);
    int chunkY = static_cast<int>(chunkPos.y / CHUNK_SIZE);
    int chunkZ = static_cast<int>(chunkPos.z / CHUNK_SIZE);
    
    std::shared_ptr<Chunk> neighborXPos = m_neighbors[0];
    std::shared_ptr<Chunk> neighborXNeg = m_neighbors[1];
    std::shared_ptr<Chunk> neighborYPos = m_neighbors[2];
    std::shared_ptr<Chunk> neighborYNeg = m_neighbors[3];
    std::shared_ptr<Chunk> neighborZPos = m_neighbors[4];
    std::shared_ptr<Chunk> neighborZNeg = m_neighbors[5];

    // Temporary arrays to store visibility and block type information
    // Each entry will store -1 for empty/hidden faces, or a value >=0 representing the block type
    std::vector<int> visibilityMask(CHUNK_SIZE * CHUNK_SIZE, -1);
    
    // Process each of the 6 face directions
    processGreedyDirection(Direction::TOP, neighborYPos);
    processGreedyDirection(Direction::BOTTOM, neighborYNeg);
    processGreedyDirection(Direction::FRONT, neighborZNeg);
    processGreedyDirection(Direction::BACK, neighborZPos);
    processGreedyDirection(Direction::LEFT, neighborXNeg);
    processGreedyDirection(Direction::RIGHT, neighborXPos);

    flags |= ChunkFlags::MESH_GENERATED;
    flags &= ~ChunkFlags::UP_TO_DATE;
}

// Helper method to process greedy meshing for a specific direction
void Chunk::processGreedyDirection(Direction direction, std::shared_ptr<Chunk> neighbor) {
    // Arrays to store visibility and block type information
    // Each entry will store -1 for empty/hidden faces, or a value >=0 representing the block type
    std::vector<int> visibilityMask(CHUNK_SIZE * CHUNK_SIZE, -1);
    
    // Define axis indices based on direction
    int normalAxis; // The axis perpendicular to the face (0=X, 1=Y, 2=Z)
    int uAxis, vAxis; // The axes parallel to the face
    int normalDirection; // 1 for positive direction, -1 for negative
    
    switch (direction) {
        case Direction::RIGHT: // Positive X
            normalAxis = 0; uAxis = 1; vAxis = 2; normalDirection = 1;
            break;
        case Direction::LEFT: // Negative X
            normalAxis = 0; uAxis = 1; vAxis = 2; normalDirection = -1;
            break;
        case Direction::TOP: // Positive Y
            normalAxis = 1; uAxis = 0; vAxis = 2; normalDirection = 1; 
            break;
        case Direction::BOTTOM: // Negative Y
            normalAxis = 1; uAxis = 0; vAxis = 2; normalDirection = -1;
            break;
        case Direction::BACK: // Positive Z
            normalAxis = 2; uAxis = 0; vAxis = 1; normalDirection = 1;
            break;
        case Direction::FRONT: // Negative Z
            normalAxis = 2; uAxis = 0; vAxis = 1; normalDirection = -1;
            break;
    }
    
    // Process each slice along the normal axis
    for (int n = 0; n < CHUNK_SIZE; n++) {
        // Clear mask for the current slice
        std::fill(visibilityMask.begin(), visibilityMask.end(), -1);
        
        // Build the visibility mask for this slice
        for (int v = 0; v < CHUNK_SIZE; v++) {
            for (int u = 0; u < CHUNK_SIZE; u++) {
                // Convert u,v,n coordinates to x,y,z based on the face direction
                int x = (normalAxis == 0) ? n : ((uAxis == 0) ? u : v);
                int y = (normalAxis == 1) ? n : ((uAxis == 1) ? u : v);
                int z = (normalAxis == 2) ? n : ((uAxis == 2) ? u : v);
                
                // Check if this face needs to be rendered
                Block block = getBlock(x, y, z);
                
                if (block.type != BlockType::AIR) {
                    // Calculate the coordinates of the adjacent block
                    int nx = x + (normalAxis == 0 ? normalDirection : 0);
                    int ny = y + (normalAxis == 1 ? normalDirection : 0);
                    int nz = z + (normalAxis == 2 ? normalDirection : 0);
                    
                    // Check if the adjacent block is air (or out of bounds)
                    bool faceVisible = false;
                    
                    if (isInBounds(nx, ny, nz)) {
                        // Adjacent block is within this chunk
                        faceVisible = (getBlock(nx, ny, nz).type == BlockType::AIR);
                    } else {
                        // Adjacent block is in a neighboring chunk
                        if (neighbor) {
                            // Convert to neighbor's coordinates
                            int neighborX = (nx < 0) ? CHUNK_SIZE - 1 : ((nx >= CHUNK_SIZE) ? 0 : nx);
                            int neighborY = (ny < 0) ? CHUNK_SIZE - 1 : ((ny >= CHUNK_SIZE) ? 0 : ny);
                            int neighborZ = (nz < 0) ? CHUNK_SIZE - 1 : ((nz >= CHUNK_SIZE) ? 0 : nz);
                            
                            faceVisible = (neighbor->getBlock(neighborX, neighborY, neighborZ).type == BlockType::AIR);
                        } else {
                            // No neighbor chunk, so face is visible
                            faceVisible = true;
                        }
                    }
                    
                    if (faceVisible) {
                        // Store the block type in the mask (convert enum to integer)
                        visibilityMask[u + v * CHUNK_SIZE] = static_cast<int>(block.type);
                    }
                }
            }
        }
        
        // Apply greedy meshing to this slice
        // This loop continues until all visible faces in this slice have been processed
        for (int v = 0; v < CHUNK_SIZE; v++) {
            for (int u = 0; u < CHUNK_SIZE; u++) {
                int blockTypeValue = visibilityMask[u + v * CHUNK_SIZE];
                
                // Skip if this face is hidden or already processed
                if (blockTypeValue == -1) continue;
                
                // Convert back to block type
                BlockType blockType = static_cast<BlockType>(blockTypeValue);
                
                // Find the width of this quad (how far it extends in the u direction)
                int width = 1;
                while (u + width < CHUNK_SIZE && visibilityMask[u + width + v * CHUNK_SIZE] == blockTypeValue) {
                    width++;
                }
                
                // Find the height of this quad (how far it extends in the v direction)
                int height = 1;
                bool canExtendHeight = true;
                
                while (canExtendHeight && v + height < CHUNK_SIZE) {
                    // Check if we can extend by one row
                    for (int du = 0; du < width; du++) {
                        if (visibilityMask[(u + du) + (v + height) * CHUNK_SIZE] != blockTypeValue) {
                            canExtendHeight = false;
                            break;
                        }
                    }
                    
                    if (canExtendHeight) height++;
                }
                
                // Mark these faces as processed
                for (int dv = 0; dv < height; dv++) {
                    for (int du = 0; du < width; du++) {
                        visibilityMask[(u + du) + (v + dv) * CHUNK_SIZE] = -1;
                    }
                }
                
                // Add the quad to the mesh
                addGreedyFace(n, u, v, width, height, blockType, direction, normalAxis, uAxis, vAxis);
            }
        }
    }
}

// Helper method to add a greedy face to the mesh
void Chunk::addGreedyFace(int normal, int u, int v, int width, int height, BlockType blockType, Direction direction, 
                           int normalAxis, int uAxis, int vAxis) {
    uint32_t vertexOffset = static_cast<uint32_t>(m_vertices.size());
    
    // Determine color based on block type (same as in addBlockFace)
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
    
    // Compute normal vector
    glm::vec3 normalVector(0.0f);
    if (normalAxis == 0) normalVector.x = (direction == Direction::RIGHT) ? 1.0f : -1.0f;
    else if (normalAxis == 1) normalVector.y = (direction == Direction::TOP) ? 1.0f : -1.0f;
    else if (normalAxis == 2) normalVector.z = (direction == Direction::BACK) ? 1.0f : -1.0f;
    
    // Calculate the coordinates for the four corners of the face
    std::array<glm::vec3, 4> positions;
    std::array<glm::vec2, 4> uvs = {
        glm::vec2(0.0f, 0.0f),
        glm::vec2(static_cast<float>(width), 0.0f),
        glm::vec2(static_cast<float>(width), static_cast<float>(height)),
        glm::vec2(0.0f, static_cast<float>(height))
    };
    
    glm::vec3 pos;
    
    // Set position for base corner
    pos[normalAxis] = static_cast<float>(normal);
    if (direction == Direction::RIGHT || direction == Direction::TOP || direction == Direction::BACK) {
        pos[normalAxis] += 1.0f;
    }
    pos[uAxis] = static_cast<float>(u);
    pos[vAxis] = static_cast<float>(v);
    positions[0] = pos;
    
    // Set position for second corner (u+width, v)
    pos[uAxis] = static_cast<float>(u + width);
    positions[1] = pos;
    
    // Set position for third corner (u+width, v+height)
    pos[vAxis] = static_cast<float>(v + height);
    positions[2] = pos;
    
    // Set position for fourth corner (u, v+height)
    pos[uAxis] = static_cast<float>(u);
    positions[3] = pos;

    // Create vertices for the quad
    std::vector<Model::Vertex> faceVertices(4);
    
    // Different face directions require different vertex orders to ensure proper winding
    const int indices[6] = {0, 1, 2, 0, 2, 3};
    int vertexOrder[4];
    
    switch (direction) {
        case Direction::TOP:
            vertexOrder[0] = 0; vertexOrder[1] = 1; vertexOrder[2] = 2; vertexOrder[3] = 3;
            break;
        case Direction::BOTTOM:
            vertexOrder[0] = 3; vertexOrder[1] = 2; vertexOrder[2] = 1; vertexOrder[3] = 0;
            break;
        case Direction::FRONT:
            vertexOrder[0] = 0; vertexOrder[1] = 1; vertexOrder[2] = 2; vertexOrder[3] = 3;
            break;
        case Direction::BACK:
            vertexOrder[0] = 3; vertexOrder[1] = 2; vertexOrder[2] = 1; vertexOrder[3] = 0;
            break;
        case Direction::LEFT:
            vertexOrder[0] = 0; vertexOrder[1] = 1; vertexOrder[2] = 2; vertexOrder[3] = 3;
            break;
        case Direction::RIGHT:
            vertexOrder[0] = 3; vertexOrder[1] = 2; vertexOrder[2] = 1; vertexOrder[3] = 0;
            break;
    }
    
    for (int i = 0; i < 4; i++) {
        faceVertices[i].position = positions[vertexOrder[i]];
        faceVertices[i].color = color;
        faceVertices[i].normal = normalVector;
        faceVertices[i].uv = uvs[vertexOrder[i]];
        faceVertices[i].block_type = static_cast<uint32_t>(blockType);
    }
    
    m_vertices.insert(m_vertices.end(), faceVertices.begin(), faceVertices.end());
    
    // Add indices for the two triangles that make up the quad
    m_indices.push_back(vertexOffset);
    m_indices.push_back(vertexOffset + 1);
    m_indices.push_back(vertexOffset + 2);
    m_indices.push_back(vertexOffset);
    m_indices.push_back(vertexOffset + 2);
    m_indices.push_back(vertexOffset + 3);
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

    flags |= ChunkFlags::UP_TO_DATE;
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
                {{bx, by + 1.0f, bz + 1.0f}, color, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, static_cast<uint32_t>(blockType)},
                {{bx + 1.0f, by + 1.0f, bz + 1.0f}, color, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, static_cast<uint32_t>(blockType)},
                {{bx + 1.0f, by + 1.0f, bz}, color, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, static_cast<uint32_t>(blockType)},
                {{bx, by + 1.0f, bz}, color, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, static_cast<uint32_t>(blockType)}
            };
            break;
        }
        case Direction::BOTTOM: {
            faceVertices = {
                {{bx, by, bz}, color, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, static_cast<uint32_t>(blockType)},
                {{bx + 1.0f, by, bz}, color, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, static_cast<uint32_t>(blockType)},
                {{bx + 1.0f, by, bz + 1.0f}, color, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, static_cast<uint32_t>(blockType)},
                {{bx, by, bz + 1.0f}, color, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, static_cast<uint32_t>(blockType)}
            };
            break;
        }
        case Direction::FRONT: {
            faceVertices = {
                {{bx, by + 1.0f, bz}, color, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, static_cast<uint32_t>(blockType)},
                {{bx + 1.0f, by + 1.0f, bz}, color, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, static_cast<uint32_t>(blockType)},
                {{bx + 1.0f, by, bz}, color, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, static_cast<uint32_t>(blockType)},
                {{bx, by, bz}, color, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, static_cast<uint32_t>(blockType)}
            };
            break;
        }
        case Direction::BACK: {
            faceVertices = {
                {{bx, by + 1.0f, bz + 1.0f}, color, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, static_cast<uint32_t>(blockType)},
                {{bx + 1.0f, by + 1.0f, bz + 1.0f}, color, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, static_cast<uint32_t>(blockType)},
                {{bx + 1.0f, by, bz + 1.0f}, color, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, static_cast<uint32_t>(blockType)},
                {{bx, by, bz + 1.0f}, color, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, static_cast<uint32_t>(blockType)}
            };
            break;
        }
        case Direction::LEFT: {
            faceVertices = {
                {{bx, by + 1.0f, bz + 1.0f}, color, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, static_cast<uint32_t>(blockType)},
                {{bx, by + 1.0f, bz}, color, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, static_cast<uint32_t>(blockType)},
                {{bx, by, bz}, color, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, static_cast<uint32_t>(blockType)},
                {{bx, by, bz + 1.0f}, color, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, static_cast<uint32_t>(blockType)}
            };
            break;
        }
        case Direction::RIGHT: {
            faceVertices = {
                {{bx + 1.0f, by + 1.0f, bz}, color, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, static_cast<uint32_t>(blockType)},
                {{bx + 1.0f, by + 1.0f, bz + 1.0f}, color, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, static_cast<uint32_t>(blockType)},
                {{bx + 1.0f, by, bz + 1.0f}, color, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, static_cast<uint32_t>(blockType)},
                {{bx + 1.0f, by, bz}, color, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, static_cast<uint32_t>(blockType)}
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
