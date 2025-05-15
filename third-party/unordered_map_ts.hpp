#pragma once

#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <functional>

//std::unordered_map<ChunkCoord, GameObject::id_t, ChunkCoord::Hash>& newActiveChunks);
// Create a thread-safe unordered_map template fopr something like above. 

template<typename Key, typename T, typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>>
class ThreadSafeUnorderedMap {
public:
    ThreadSafeUnorderedMap() = default;
    ~ThreadSafeUnorderedMap() = default;

    // Disable copy constructor and assignment operator
    ThreadSafeUnorderedMap(const ThreadSafeUnorderedMap&) = delete;
    ThreadSafeUnorderedMap& operator=(const ThreadSafeUnorderedMap&) = delete;

    // Insert or update a key-value pair
    void insert_or_update(const Key& key, const T& value) {
        std::unique_lock lock(mtx);
        map[key] = value;
    }

    T get(const Key& key) {
        std::shared_lock lock(mtx);
        return map.at(key);
    }

    bool contains(const Key& key) {
        std::shared_lock lock(mtx);
        return map.find(key) != map.end();
    }

    void remove(const Key& key) {
        std::unique_lock lock(mtx);
        map.erase(key);
    }

    void clear() {
        std::unique_lock lock(mtx);
        map.clear();
    }

    size_t size() const {
        std::shared_lock lock(mtx);
        return map.size();
    }

    bool empty() const {
        std::shared_lock lock(mtx);
        return map.empty();
    }

    void for_each(const std::function<void(const Key&, const T&)>& func) {
        std::shared_lock lock(mtx);
        for (const auto& pair : map) {
            func(pair.first, pair.second);
        }
    }
    
    std::unordered_map<Key, T, Hash, KeyEqual> get_map() {
        std::shared_lock lock(mtx);
        return map;
    }

    // Thread-safe operator[] for non-const access (inserts default if not found)
    T& operator[](const Key& key) {
        std::unique_lock lock(mtx);
        return map[key];
    }

    // Thread-safe operator[] for move semantics
    T& operator[](Key&& key) {
        std::unique_lock lock(mtx);
        return map[std::move(key)];
    }
private:
    mutable std::shared_mutex mtx;
    std::unordered_map<Key, T, Hash, KeyEqual> map;
};