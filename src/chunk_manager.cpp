#include "../include/chunk_manager.hpp"

namespace vkengine {

ChunkManager::ChunkManager(Device& deviceRef) : device{deviceRef} {
    // Initialize the chunk manager
}

ChunkManager::~ChunkManager() {
    // Clean up resources
}

void ChunkManager::update(const glm::vec3& playerPos, int viewDistance, GameObject::Map& gameObjects) {
    // Convert world position to chunk coordinates
    ChunkCoord centerChunk = worldToChunkCoord(playerPos);
    
    // Create a set for new active chunks
    std::unordered_map<ChunkCoord, GameObject::id_t, ChunkCoord::Hash> newActiveChunks;
    
    // Define vertical view range (how many chunks up and down to render)
    // Using a smaller value for vertical range since worlds are typically wider than tall
    int verticalViewRange = viewDistance / 2 + 1;  // Adjust as needed
    
    // Generate/update chunks within view distance in 3D space
    for (int x = centerChunk.x - viewDistance; x <= centerChunk.x + viewDistance; x++) {
        for (int y = centerChunk.y - verticalViewRange; y <= centerChunk.y + verticalViewRange; y++) {
            for (int z = centerChunk.z - viewDistance; z <= centerChunk.z + viewDistance; z++) {
                ChunkCoord coord{x, y, z};
                
                // Check if the chunk is within the view distance (using squared distance for efficiency)
                if (isChunkInRange(coord, centerChunk, viewDistance)) {
                    // Check if chunk already exists
                    auto it = m_chunks.find(coord);
                    std::shared_ptr<Chunk> chunk;
                    
                    if (it == m_chunks.end()) {
                        // Create new chunk
                        chunk = createChunk(coord);
                        m_chunks[coord] = chunk;
                    } else {
                        // Use existing chunk
                        chunk = it->second;
                    }
                    
                    // Get game object ID and add to new active chunks
                    GameObject::id_t objectId = chunk->getGameObject()->getId();
                    newActiveChunks[coord] = objectId;
                    
                    // Add to game objects map if not already there
                    if (gameObjects.find(objectId) == gameObjects.end()) {
                        gameObjects.emplace(objectId, chunk->getGameObject());
                    }
                }
            }
        }
    }
    
    // Remove game objects for chunks that are no longer active
    for (const auto& [coord, objectId] : m_activeChunks) {
        if (newActiveChunks.find(coord) == newActiveChunks.end()) {
            gameObjects.erase(objectId);
        }
    }
    
    // Update active chunks
    m_activeChunks = newActiveChunks;
}

ChunkCoord ChunkManager::worldToChunkCoord(const glm::vec3& position) {
    // Convert world coordinates to chunk coordinates
    return {
        static_cast<int>(std::floor(position.x / CHUNK_SIZE)),
        static_cast<int>(std::floor(position.y / CHUNK_SIZE)),
        static_cast<int>(std::floor(position.z / CHUNK_SIZE))
    };
}

bool ChunkManager::isChunkInRange(const ChunkCoord& chunkCoord, const ChunkCoord& centerChunk, int viewDistance) {
    // Calculate squared distance between chunks in 3D space
    int dx = chunkCoord.x - centerChunk.x;
    int dy = chunkCoord.y - centerChunk.y;
    int dz = chunkCoord.z - centerChunk.z;
    int squaredDistance = dx * dx + dy * dy + dz * dz;
    
    // Compare with squared view distance
    return squaredDistance <= viewDistance * viewDistance;
}

std::shared_ptr<Chunk> ChunkManager::createChunk(const ChunkCoord& coord) {
    // Create game object for chunk
    auto gameObject = GameObject::createGameObject();
    
    // Set position based on 3D chunk coordinates
    gameObject->transform.translation = {
        static_cast<float>(coord.x * CHUNK_SIZE),
        static_cast<float>(coord.y * CHUNK_SIZE),
        static_cast<float>(coord.z * CHUNK_SIZE)
    };
    
    // Create chunk
    auto chunk = std::make_shared<Chunk>(device, gameObject);
    
    // Generate terrain and mesh
    chunk->generateTerrain();
    chunk->generateMesh();
    chunk->updateGameObject();
    
    return chunk;
}

} // namespace vkengine
