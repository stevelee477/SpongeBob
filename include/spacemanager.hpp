#ifndef __SPACE_MANAGER__
#define __SPACE_MANAGER__


#include "file.hpp"
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>
#include <unordered_set>
#include <iostream>

class SpaceManager {
public:
    SpaceManager() = delete;
    SpaceManager(uint64_t space_start, uint64_t space_end, uint64_t block_size, uint64_t server_id_);
    ~SpaceManager() = default;
    std::vector<uint64_t> AllocateSpace(uint64_t length);
    uint64_t AllocateOneBlock();
    uint64_t GetSpaceStart() { return space_start_; }
    bool ReclaimInodeSpace(std::shared_ptr<Inode> inode);
    bool ReclaimSpace(std::vector<uint64_t>& block_list);
    void ResetSpaceRange(uint64_t start_addr, uint64_t length);
    inline uint64_t GetFileBlockSize() { return block_size_; }

private:
    std::unordered_set<uint64_t> inused_blocks_;
    std::priority_queue<uint64_t, std::vector<uint64_t>, std::greater<uint64_t>> free_list_;
    std::mutex space_lock_;
    uint64_t space_start_;
    uint64_t space_end_;

    uint64_t total_bytes_;
    uint64_t block_size_;
    uint64_t total_blocks_;
    uint64_t cur_blocks_;

    uint64_t server_id_;
};


#endif