#include "ChunkCache.h"

void ChunkCache::insert(int chunkId, std::shared_ptr<GltfChunk> chunk) {
    auto& entry = chunks_[chunkId];
    std::unique_lock lock(entry.mutex);
    entry.data = std::move(chunk);
}

std::shared_ptr<GltfChunk> ChunkCache::get(int chunkId) {
    auto it = chunks_.find(chunkId);
    if (it == chunks_.end()) return nullptr;
    std::shared_lock lock(it->second.mutex);
    return it->second.data;
}
