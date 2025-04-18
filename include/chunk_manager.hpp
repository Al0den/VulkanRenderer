#pragma once

#include "chunk.hpp"
#include "device.hpp"
#include "game_object.hpp"

#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <condition_variable>

namespace vkengine {

// Structure to represent chunk coordinates
struct ChunkCoord {
    int x;
    int y;
    int z;
    
    bool operator==(const ChunkCoord& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
    
    // Hash function for ChunkCoord to use in unordered_map
    struct Hash {
        std::size_t operator()(const ChunkCoord& coord) const {
            return std::hash<int>()(coord.x) ^ 
                   (std::hash<int>()(coord.y) << 1) ^
                   (std::hash<int>()(coord.z) << 2);
        }
    };
};

class ChunkManager {
public:
    ChunkManager(Device& deviceRef);
    ~ChunkManager();

    // Update chunks based on player position and view distance
    // Adds/removes chunks to the provided gameObjects map as needed
    void update(const glm::vec3& playerPos, int viewDistance, GameObject::Map& gameObjects);
    
    // Convert world position to chunk coordinates
    ChunkCoord worldToChunkCoord(const glm::vec3& position);
    
    // Check if a chunk is within view distance range
    bool isChunkInRange(const ChunkCoord& chunkCoord, const ChunkCoord& centerChunk, int viewDistance);
    
    // Create a new chunk at the specified coordinates
    std::shared_ptr<Chunk> createChunk(const ChunkCoord& coord);

private:
    Device& device;
    
    // Map from chunk coordinates to chunk objects
    std::unordered_map<ChunkCoord, std::shared_ptr<Chunk>, ChunkCoord::Hash> m_chunks;
    
    // Map from chunk coordinates to game object IDs for active chunks
    std::unordered_map<ChunkCoord, GameObject::id_t, ChunkCoord::Hash> m_activeChunks;
    
    // Thread-safe queue for chunks waiting to be processed
    struct ChunkTask {
        std::shared_ptr<Chunk> chunk;
        bool needsTerrainGeneration;
        bool needsMeshUpdate;
    };
    std::queue<ChunkTask> m_chunkQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCondition;
    
    // Worker thread control
    std::thread m_workerThread;
    std::atomic<bool> m_threadRunning{true};
    
    void updateChunk(std::shared_ptr<Chunk> chunk);
    void generateTerrain(std::shared_ptr<Chunk> chunk);
    
    // Worker thread function that processes chunks in the queue
    void workerThreadFunction();
    
    // Queue a chunk for processing
    void queueChunkForProcessing(std::shared_ptr<Chunk> chunk, bool needsTerrainGeneration, bool needsMeshUpdate);
};

} // namespace vkengine
