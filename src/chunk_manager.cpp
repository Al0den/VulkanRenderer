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
    waitForThreads();
}

std::shared_ptr<Chunk> ChunkManager::queueChunkCreation(const ChunkCoord& coord) {  
    std::shared_lock<std::shared_mutex> lock(chunksMutex);
    auto it = m_chunks.find(coord);
    lock.unlock();
    
    if (it == m_chunks.end()) {
        if(flags & ChunkManagerFlags::GENERATE_CHUNKS) {
            std::lock_guard<std::mutex> lock(creationMutex);
            chunksNeedingCreating.push(coord);
            creationSemaphore.release();
        }
        return nullptr;
    } else {
        return it->second;
    }
}
bool ChunkManager::queueChunkTerrainGeneration(std::shared_ptr<Chunk> chunk) {
    ScopeTimer timer("ChunkManager::generateTerrain");
    if(!chunk->defaultTerrainGenerated()) {
        {
            std::lock_guard<std::mutex> lock(terrainMutex);
            chunksNeedingTerrainGeneration.push(chunk);
            terrainSemaphore.release();
        }
        return false;
    }
    return true;
}

bool ChunkManager::queueChunkMeshGeneration(std::shared_ptr<Chunk> chunk) {
    ScopeTimer timer("ChunkManager::generateMesh");
    if(!chunk->meshGenerated()) {
        {
            std::lock_guard<std::mutex> lock(meshMutex);
            chunksNeedingMeshUpdate.push(chunk);
            meshSemaphore.release();
        }
        return false;

    }

    return true;
}

bool ChunkManager::updateGameObject(std::shared_ptr<Chunk> chunk) {
    {
        ScopeTimer timer("ChunkManager::updateGameObject");
        if(!chunk->upToDate()) {
            chunk->updateGameObject();
        }
    }
    return true;
}

bool ChunkManager::updateActiveChunks(GameObject::Map& gameObjects, std::shared_ptr<Chunk> chunk) {
    {
        ScopeTimer timer("ChunkManager::updateActiveChunks");
        GameObject::id_t objectId = chunk->getGameObject()->getId();
        ChunkCoord coord = chunk->getChunkCoord();

        newChunks.push(chunk);
        if (gameObjects.find(objectId) == gameObjects.end()) {
            gameObjects.emplace(objectId, chunk->getGameObject());
        }
    }
    return true;
}

void ChunkManager::loopOverChunksThread(const glm::vec3& playerPos, int viewDistance, GameObject::Map& gameObjects) {
    ScopeTimer timer("ChunkManager::loopOverChunks");
    
    ChunkCoord centerChunk = worldToChunkCoord(playerPos);
    
    int verticalViewRange = viewDistance / 2 + 1;  // Adjust as needed
    
    for (int x = centerChunk.x - viewDistance; x <= centerChunk.x + viewDistance; x++) {
        for (int y = centerChunk.y - verticalViewRange; y <= centerChunk.y + verticalViewRange; y++) {
            for (int z = centerChunk.z - viewDistance; z <= centerChunk.z + viewDistance; z++) {
                ChunkCoord coord{x, y, z};
                
                if (isChunkInRange(coord, centerChunk, viewDistance)) {
                    std::shared_ptr<Chunk> chunk = queueChunkCreation(coord);
                    if(chunk == nullptr) { continue; }

                    if(!queueChunkTerrainGeneration(chunk)) { continue; }
                    if(!queueChunkMeshGeneration(chunk)) { continue; }
                    if(!updateGameObject(chunk)) { continue; }
                    if(!updateActiveChunks(gameObjects, chunk)) { continue; }
                }
            }
        }
    }
}

void ChunkManager::update(const glm::vec3& playerPos, int viewDistance, GameObject::Map& gameObjects) {
    ChunkCoord centerChunk = worldToChunkCoord(playerPos);
    
    int verticalViewRange = viewDistance / 2 + 1;  // Adjust as needed
    
    for (int x = centerChunk.x - viewDistance; x <= centerChunk.x + viewDistance; x++) {
        for (int y = centerChunk.y - verticalViewRange; y <= centerChunk.y + verticalViewRange; y++) {
            for (int z = centerChunk.z - viewDistance; z <= centerChunk.z + viewDistance; z++) {
                ChunkCoord coord{x, y, z};
                
                if (isChunkInRange(coord, centerChunk, viewDistance)) {
                    std::shared_ptr<Chunk> chunk = queueChunkCreation(coord);
                    if(chunk == nullptr) { continue; }

                    if(!queueChunkTerrainGeneration(chunk)) { continue; }
                    if(!queueChunkMeshGeneration(chunk)) { continue; }
                    if(!updateGameObject(chunk)) { continue; }
                    if(!updateActiveChunks(gameObjects, chunk)) { continue; }
                }
            }
        }
    }

    ScopeTimer timer("ChunkManager::updateActiveChunks");

    newChunksMutex.lock();
    while (!newChunks.empty()) {
        auto chunk = newChunks.front();
        newChunks.pop();
        ChunkCoord coord = chunk->getChunkCoord();
        
        if (m_activeChunks.find(coord) == m_activeChunks.end()) {
            GameObject::id_t objectId = chunk->getGameObject()->getId();
            m_activeChunks[coord] = objectId;
            gameObjects[objectId] = chunk->getGameObject();
        }
    }
    newChunksMutex.unlock();
    
    std::vector<ChunkCoord> chunksToRemove;
    for (const auto& [coord, objectId] : m_activeChunks) {
        if(isChunkInRange(coord, centerChunk, viewDistance)) {
            continue;
        } else {
            chunksToRemove.push_back(coord);
        }
    }

    for (const auto& coord : chunksToRemove) {
        GameObject::id_t objectId = m_activeChunks[coord];
        m_activeChunks.erase(coord);
        gameObjects.erase(objectId);
    }
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
    stopThreads = true;
    
    // Notify all semaphores to wake up threads
    terrainSemaphore.release(numTerrainThreads);
    meshSemaphore.release(numMeshThreads);
    creationSemaphore.release(numCreationThreads);
    
    // Join all threads
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    threads.clear();
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

std::string ChunkManager::serialize() const {
    std::string data;
    for (const auto& chunkPair : m_chunks) {
        data += chunkPair.second->serialize() + "\n";
    }
    return data;
}

void ChunkManager::deserialize(const std::string& data) {
    // Clear existing chunks
    m_chunks.clear();
    m_activeChunks.clear();
    std::istringstream iss(data);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (!line.empty()) {
            auto chunk = std::make_shared<Chunk>(device, GameObject::createGameObject());
            chunk->deserialize(line);
            m_chunks[chunk->getChunkCoord()] = chunk;
        }
    }
}
}