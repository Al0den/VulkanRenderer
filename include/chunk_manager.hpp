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
#include <shared_mutex>

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

    std::shared_ptr<Chunk> queueChunkCreation(const ChunkCoord& coord);
    bool queueChunkTerrainGeneration(std::shared_ptr<Chunk> chunk);
    bool queueChunkMeshGeneration(std::shared_ptr<Chunk> chunk);
    bool updateGameObject(std::shared_ptr<Chunk> chunk);
    bool updateActiveChunks(std::unordered_map<ChunkCoord, GameObject::id_t, ChunkCoord::Hash>& newActiveChunks, GameObject::Map& gameObjects, std::shared_ptr<Chunk> chunk);


    void regenerateEntireMesh();

    std::string serialize() const;
    void deserialize(const std::string& data);

    int flags = ChunkManagerFlags::GENERATE_CHUNKS;

private:
    int currentViewDistance = 2;
    Device& device;
    
    std::unordered_map<ChunkCoord, GameObject::id_t, ChunkCoord::Hash> m_activeChunks;

    std::queue<ChunkCoord> chunksNeedingCreating;
    std::queue<std::shared_ptr<Chunk>> chunksNeedingTerrainGeneration;
    std::queue<std::shared_ptr<Chunk>> chunksNeedingMeshUpdate;
    std::queue<std::shared_ptr<Chunk>> chunksNeedingPush;
    std::queue<std::shared_ptr<Chunk>> newChunks;

    std::vector<std::thread> threads;

    std::shared_mutex chunksMutex;

    int numCreationThreads = 8;
    int numTerrainThreads = 8;
    int numMeshThreads = 8;

    void chunksTerrainGenerationThread();
    void chunksMeshUpdateThread();
    void chunksCreationThread();
    void chunksPushThread();

    std::atomic<bool> stopThreads{false};

    std::counting_semaphore<1024> creationSemaphore{0};
    std::counting_semaphore<1024> terrainSemaphore{0};
    std::counting_semaphore<1024> meshSemaphore{0};
    std::counting_semaphore<1024> pushSemaphore{0};
    
    std::mutex creationMutex;
    std::mutex terrainMutex;
    std::mutex meshMutex;
    std::mutex pushMutex;
    std::mutex newChunksMutex;
    
    void stopAllThreads();

    void waitForThreads();
};

} // namespace vkengine
