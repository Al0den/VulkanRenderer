#pragma once

#include "chunk.hpp"
#include "device.hpp"
#include "game_object.hpp"

#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <semaphore>
#include <thread>
#include <mutex>

#include <queue>
#include <atomic>
#include <condition_variable>

namespace vkengine {

class ChunkManager {
public:
    ChunkManager(Device& deviceRef);
    ~ChunkManager();

    void update(const glm::vec3& playerPos, int viewDistance, GameObject::Map& gameObjects);
    
    ChunkCoord worldToChunkCoord(const glm::vec3& position);
    
    bool isChunkInRange(const ChunkCoord& chunkCoord, const ChunkCoord& centerChunk, int viewDistance);
    
    std::shared_ptr<Chunk> createChunk(const ChunkCoord& coord);
    

    std::unordered_map<ChunkCoord, std::shared_ptr<Chunk>, ChunkCoord::Hash> m_chunks;

private:
    Device& device;
    
    std::unordered_map<ChunkCoord, GameObject::id_t, ChunkCoord::Hash> m_activeChunks;

    std::queue<std::shared_ptr<Chunk>> chunksNeedingCreating;
    std::queue<std::shared_ptr<Chunk>> chunksNeedingTerrainGeneration;
    std::queue<std::shared_ptr<Chunk>> chunksNeedingMeshUpdate;

    std::vector<std::thread> threads;

    std::mutex chunksMutex;

    int numCreationThreads = 4;
    int numTerrainThreads = 4;
    int numMeshThreads = 4;

    void chunksTerrainGenerationThread();
    void chunksMeshUpdateThread();

    std::atomic<bool> stopThreads{false};

    std::counting_semaphore<1024> terrainSemaphore{0};
    std::counting_semaphore<1024> meshSemaphore{0};

    std::mutex terrainMutex;
    std::mutex meshMutex;
    
    void stopAllThreads();

    void waitForThreads();
    
};

} // namespace vkengine
