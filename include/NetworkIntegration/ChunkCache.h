#pragma once
#include "GltfChunk.h"
#include <unordered_map>
#include <shared_mutex>
#include <memory>

class ChunkCache {
public:
    void insert(int chunkId, std::shared_ptr<GltfChunk> chunk);
    std::shared_ptr<GltfChunk> get(int chunkId);
private:
    struct Entry {
        std::shared_ptr<GltfChunk> data;
        std::shared_mutex mutex;
    };
    std::unordered_map<int, Entry> chunks_;
    std::shared_mutex global_mutex_; // guards insert/find consistency
};