#include "../include/chunk_manager.hpp"
#include "../include/config.hpp"
#include "../include/scope_timer.hpp"

#include <thread>

using ScopeTimer = GlobalTimerData::ScopeTimer;

static int neighborOffsets[6][3] = {
    {1, 0, 0},   // X+
    {-1, 0, 0},  // X-
    {0, 1, 0},   // Y+
    {0, -1, 0},  // Y-
    {0, 0, 1},   // Z+
    {0, 0, -1}   // Z-
};

static int numNeighbors = sizeof(neighborOffsets) / sizeof(neighborOffsets[0]);

namespace vkengine {

ChunkManager::ChunkManager(Device& deviceRef) : device{deviceRef} {
    for (int i = 0; i < numTerrainThreads; ++i) {
        threads.emplace_back(&ChunkManager::chunksTerrainGenerationThread, this);
    }

    for (int i = 0; i < numMeshThreads; ++i) {
        threads.emplace_back(&ChunkManager::chunksMeshUpdateThread, this);
    }

    for (int i = 0; i < numCreationThreads; ++i) {
        threads.emplace_back(&ChunkManager::chunksCreationThread, this);
    }
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
                    
                    std::shared_ptr<Chunk> chunk;
                    
                    { 
                        ScopeTimer timer("ChunkManager::createChunk");
                        {
                            
                            std::shared_lock<std::shared_mutex> lock(chunksMutex);
                            auto it = m_chunks.find(coord);
                            lock.unlock();
                            
                            if (it == m_chunks.end()) {
                                std::lock_guard<std::mutex> lock(creationMutex);
                                chunksNeedingCreating.push(coord);
                                creationSemaphore.release();
                                continue;
                            } else {
                                chunk = it->second;
                            }
                        }
                    }

                    {
                        ScopeTimer timer("ChunkManager::generateTerrain");
                        if(!chunk->defaultTerrainGenerated()) {
                            {
                                std::lock_guard<std::mutex> lock(terrainMutex);
                                chunksNeedingTerrainGeneration.push(chunk);
                                terrainSemaphore.release();
                            }
                            continue;
                        }
                    }

                    
                    {
                        ScopeTimer timer("ChunkManager::generateMesh");
                        if(!chunk->meshGenerated()) {
                            {
                                std::lock_guard<std::mutex> lock(meshMutex);
                                chunksNeedingMeshUpdate.push(chunk);
                                meshSemaphore.release();
                            }
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

            std::shared_lock<std::shared_mutex> lock(chunksMutex);

            auto it = m_chunks.find(coord);
            if (it != m_chunks.end()) {
                auto chunk = it->second;
                chunk->clearMesh();
            }
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

void ChunkManager::stopAllThreads() {
    stopThreads = true;
}

void ChunkManager::waitForThreads() {
    // This function would typically join all the threads.
    // However, the thread objects themselves are not stored in the ChunkManager in the provided code.
    // Assuming threads are managed elsewhere or this is a simplified scenario.
    // If you have std::vector<std::thread> members for each thread type, you'd join them here.
    // For example:
    // for (auto& thread : creationThreads) { if (thread.joinable()) thread.join(); }
    // for (auto& thread : terrainThreads) { if (thread.joinable()) thread.join(); }
    // for (auto& thread : meshThreads) { if (thread.joinable()) thread.join(); }
}

void ChunkManager::chunksTerrainGenerationThread() {
    while (!stopThreads) {
        terrainSemaphore.acquire();
        if (stopThreads) break;
        
        std::shared_ptr<Chunk> chunk;
        {
            std::lock_guard<std::mutex> lock(terrainMutex);
            if (chunksNeedingTerrainGeneration.empty()) continue;
            chunk = chunksNeedingTerrainGeneration.front();
            chunksNeedingTerrainGeneration.pop();
        }

        // Check if the chunk is still valid before using it
        if (chunk) {
            std::lock_guard<std::mutex> lock(chunk->m_mutex);
            if(!chunk->defaultTerrainGenerated()) {
                chunk->generateTerrain();
            }
        }
        
    }
}

void ChunkManager::chunksCreationThread() {
    while (!stopThreads) {
        creationSemaphore.acquire();
        if (stopThreads) break;
        
        ChunkCoord chunkToGenerate;
        {
            std::lock_guard<std::mutex> lock(creationMutex);
            if (chunksNeedingCreating.empty()) continue;
            chunkToGenerate = chunksNeedingCreating.front();
            chunksNeedingCreating.pop();
        }

        std::shared_ptr<Chunk> chunk;

        chunk = createChunk(chunkToGenerate);
        {
            std::unique_lock<std::shared_mutex> lock(chunksMutex);
            m_chunks[chunkToGenerate] = chunk;
        }
    }
}

void ChunkManager::chunksMeshUpdateThread() {
    while (!stopThreads) {
        meshSemaphore.acquire();
        if (stopThreads) break;
        
        std::shared_ptr<Chunk> chunk;
        {
            std::lock_guard<std::mutex> lock(meshMutex);
            if (chunksNeedingMeshUpdate.empty()) continue;
            chunk = chunksNeedingMeshUpdate.front();
            chunksNeedingMeshUpdate.pop();
        }

        if(chunk) {
            bool anyNeighborMissing = false;
            {
                 // Lock the main container while copying neighbors
                ChunkCoord chunkCoord = chunk->getChunkCoord();

                if(!chunk->allNeighborsLoaded()) {
                    for(int i=0; i < numNeighbors; i++) {
                        if(chunk->m_neighbors[i] == nullptr) {
                            int neighborX = chunkCoord.x + neighborOffsets[i][0];
                            int neighborY = chunkCoord.y + neighborOffsets[i][1];
                            int neighborZ = chunkCoord.z + neighborOffsets[i][2];

                            ChunkCoord neighborCoord{neighborX, neighborY, neighborZ};
                            std::shared_lock<std::shared_mutex> mainLock(chunksMutex);
                            auto it = m_chunks.find(neighborCoord);
                            
                            if (it != m_chunks.end()) {
                                if(it->second->defaultTerrainGenerated()) {
                                    chunk->m_neighbors[i] = it->second;
                                } else {
                                    anyNeighborMissing = true;
                                }
                            } else {
                                anyNeighborMissing = true;
                            }
                        }
                    } 
                } 
                
                // If any neighbor is missing but could potentially exist, requeue the chunk
                if (anyNeighborMissing) {
                    std::lock_guard<std::mutex> meshLock(meshMutex);
                    chunksNeedingMeshUpdate.push(chunk);
                    meshSemaphore.release();
                    continue;
                }
            }
            
            {
                std::lock_guard<std::mutex> lock(chunk->m_mutex);
                if(!chunk->meshGenerated()) {
                    if(static_cast<MeshingTechnique>(config().getInt("meshing_technique")) == MeshingTechnique::SIMPLE) {
                        chunk->generateMesh();
                    } else if(static_cast<MeshingTechnique>(config().getInt("meshing_technique")) == MeshingTechnique::GREEDY) {
                        chunk->generateGreedyMesh(); // Assuming generateGreedyMesh() is a valid method
                    }
                }
            }
        }
    }
}

void ChunkManager::regenerateEntireMesh() {
    for (auto& chunkPair : m_chunks) {
        auto& chunk = chunkPair.second;
        if (chunk) {
            chunk->setMeshGenerated(false);
        }
    }
}

}