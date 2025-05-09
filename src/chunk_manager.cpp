#include "../include/chunk_manager.hpp"
#include "../include/config.hpp"
#include "../include/scope_timer.hpp"

#include <thread>

using ScopeTimer = GlobalTimerData::ScopeTimer;

namespace vkengine {

void generateTerrainThread(ChunkManager *manager, std::shared_ptr<Chunk> chunk);
void generateMeshThread(ChunkManager *manager, std::shared_ptr<Chunk> chunk);
void regenerateNeighborsMesh(ChunkManager *manager, std::shared_ptr<Chunk> chunk);

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

                    {
                        ScopeTimer timer("ChunkManager::createChunk");
                        if (it == m_chunks.end()) {
                            // Create new chunk
                            chunk = createChunk(coord);
                            m_chunks[coord] = chunk;
                        } else {
                            chunk = it->second;
                        }
                    }
                    
                    {
                        ScopeTimer timer("ChunkManager::regenerateNeighborsMesh");
                        if(chunk->isUpdated()) {
                            std::thread t(regenerateNeighborsMesh, this, chunk);
                            t.detach();
                            chunk->m_updated = false;
                        }
                    }
                    
                    {
                        ScopeTimer timer("ChunkManager::generateTerrain");
                        if(!chunk->defaultTerrainGenerated()) {
                            std::thread terrainThread(generateTerrainThread, this, chunk);
                            terrainThread.detach();
                            continue;
                        }
                    }
                    
                    {
                        ScopeTimer timer("ChunkManager::generateMesh");
                        if(!chunk->meshGenerated()) {
                            std::thread meshThread(generateMeshThread, this, chunk);
                            meshThread.detach();
                            continue;
                        }
                    }
                    
                    {
                        ScopeTimer timer("ChunkManager::updateGameObject");
                        if(!chunk->upToDate()) {
                            chunk->updateGameObject();
                        }
                    }

                    {
                        ScopeTimer timer("ChunkManager::updateActiveChunks");
                        // Get game object ID and add to new active chunks
                        GameObject::id_t objectId = chunk->getGameObject()->getId();
                        newActiveChunks[coord] = objectId;
                        if (gameObjects.find(objectId) == gameObjects.end()) {
                            gameObjects.emplace(objectId, chunk->getGameObject());
                        }
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
       
    return chunk;
}

void generateTerrainThread(ChunkManager *manager, std::shared_ptr<Chunk> chunk) {
    // Generate terrain for the chunk
    {
        std::lock_guard<std::mutex> lock(manager->currentlyGeneratingChunksMutex);
        if (std::find(manager->currentlyGeneratingChunks.begin(), manager->currentlyGeneratingChunks.end(), std::hash<std::shared_ptr<Chunk>>()(chunk)) != manager->currentlyGeneratingChunks.end()) {
            return; // Already generating this chunk
        }

        manager->currentlyGeneratingChunks.push_back(std::hash<std::shared_ptr<Chunk>>()(chunk));
    }
    
    chunk->generateTerrain();

    {   
        std::lock_guard<std::mutex> lock(manager->currentlyGeneratingChunksMutex);
        manager->currentlyGeneratingChunks.erase(
            std::remove(manager->currentlyGeneratingChunks.begin(), manager->currentlyGeneratingChunks.end(), std::hash<std::shared_ptr<Chunk>>()(chunk)),
            manager->currentlyGeneratingChunks.end()
        );
    }
}

void regenerateNeighborsMesh(ChunkManager *manager, std::shared_ptr<Chunk> chunk) {
    std::shared_ptr<Chunk> neighborXPos = nullptr; // Right (x+)
    std::shared_ptr<Chunk> neighborXNeg = nullptr; // Left (x-)
    std::shared_ptr<Chunk> neighborYPos = nullptr; // Top (y+)
    std::shared_ptr<Chunk> neighborYNeg = nullptr; // Bottom (y-)
    std::shared_ptr<Chunk> neighborZPos = nullptr; // Back (z+)
    std::shared_ptr<Chunk> neighborZNeg = nullptr; // Front (z-)

    // Get this chunk's world position
    glm::vec3 chunkPos = chunk->getGameObject()->transform.translation;

    int chunkX = static_cast<int>(chunkPos.x / CHUNK_SIZE);
    int chunkY = static_cast<int>(chunkPos.y / CHUNK_SIZE);
    int chunkZ = static_cast<int>(chunkPos.z / CHUNK_SIZE);

    // Define the coordinates of the 6 potentially adjacent chunks
    ChunkCoord coordXPos{chunkX + 1, chunkY, chunkZ}; // Right
    ChunkCoord coordXNeg{chunkX - 1, chunkY, chunkZ}; // Left
    ChunkCoord coordYPos{chunkX, chunkY + 1, chunkZ}; // Top
    ChunkCoord coordYNeg{chunkX, chunkY - 1, chunkZ}; // Bottom
    ChunkCoord coordZPos{chunkX, chunkY, chunkZ + 1}; // Back
    ChunkCoord coordZNeg{chunkX, chunkY, chunkZ - 1}; // Front

    // Look up each potential neighbor in the chunks map
    auto itXPos = manager->m_chunks.find(coordXPos);
    if (itXPos != manager->m_chunks.end() && itXPos->second->defaultTerrainGenerated()) {
        itXPos->second->setMeshGenerated(false);
    }

    auto itXNeg = manager->m_chunks.find(coordXNeg);
    if (itXNeg != manager->m_chunks.end() && itXNeg->second->defaultTerrainGenerated()) {
        itXNeg->second->setMeshGenerated(false);
    }

    auto itYPos = manager->m_chunks.find(coordYPos);
    if (itYPos != manager->m_chunks.end() && itYPos->second->defaultTerrainGenerated()) {
        itYPos->second->setMeshGenerated(false);
    }

    auto itYNeg = manager->m_chunks.find(coordYNeg);
    if (itYNeg != manager->m_chunks.end() && itYNeg->second->defaultTerrainGenerated()) {
        itYNeg->second->setMeshGenerated(false);
    }

    auto itZPos = manager->m_chunks.find(coordZPos);
    if (itZPos != manager->m_chunks.end() && itZPos->second->defaultTerrainGenerated()) {
        itZPos->second->setMeshGenerated(false);
    }

    auto itZNeg = manager->m_chunks.find(coordZNeg);
    if (itZNeg != manager->m_chunks.end() && itZNeg->second->defaultTerrainGenerated()) {
        itZNeg->second->setMeshGenerated(false);
    }
}

void generateMeshThread(ChunkManager *manager, std::shared_ptr<Chunk> chunk) {
    {   
        std::lock_guard<std::mutex> lock(manager->currentlyGeneratingMeshesMutex);
        if (std::find(manager->currentlyGeneratingMeshes.begin(), manager->currentlyGeneratingMeshes.end(), std::hash<std::shared_ptr<Chunk>>()(chunk)) != manager->currentlyGeneratingMeshes.end()) {
            return; // Already generating this chunk
        }

        manager->currentlyGeneratingMeshes.push_back(std::hash<std::shared_ptr<Chunk>>()(chunk));
    }
    
    if(config().getInt("meshing_technique") == 0) {
        chunk->generateMesh(manager->m_chunks);
    } else if(config().getInt("meshing_technique") == 1) {
        chunk->generateGreedyMesh(manager->m_chunks);
    } else {
        throw std::runtime_error("Invalid meshing technique");
    }

    {
        std::lock_guard<std::mutex> lock(manager->currentlyGeneratingMeshesMutex);
        manager->currentlyGeneratingMeshes.erase(
            std::remove(manager->currentlyGeneratingMeshes.begin(), manager->currentlyGeneratingMeshes.end(), std::hash<std::shared_ptr<Chunk>>()(chunk)),
            manager->currentlyGeneratingMeshes.end()
        );
    }
}

void ChunkManager::regenerateAllMeshes() {
    
    for (auto& [coord, chunk] : m_chunks) {
        if (chunk->defaultTerrainGenerated()) {
            chunk->setMeshGenerated(false);
        }
    }
}

}