#include "spacemanager.hpp"

SpaceManager::SpaceManager(uint64_t space_start, uint64_t space_end, uint64_t block_size)
    :space_start_(space_start), space_end_(space_end), block_size_(block_size), cur_blocks_(0) {
    total_bytes_ = space_end_ - space_start_ + 1;
    total_blocks_ = total_bytes_ / block_size_;
    for (uint64_t i = 0; i < total_blocks_; ++i) {
        free_list_.push(i);
    }
}

std::vector<uint64_t> SpaceManager::AllocateSpace(uint64_t length) {
    space_lock_.lock();
    uint64_t to_allocate = (length + block_size_) / block_size_;
    if (to_allocate > total_blocks_ - cur_blocks_) {
        std::cerr << "Space isn't enough." << std::endl;
        space_lock_.unlock();
        return std::vector<uint64_t>();
    }
    std::vector<uint64_t> res(to_allocate);
    for (uint64_t i = 0; i < to_allocate; ++i) {
        res[i] = free_list_.top();
        free_list_.pop();
        inused_blocks_.insert(res[i]);
        cur_blocks_++;
    }
    space_lock_.unlock();
    return res;
}

bool SpaceManager::ReclaimSpace(std::vector<uint64_t> &block_list) {
    space_lock_.lock();
    for (uint64_t block_nr: block_list) {
        free_list_.push(block_nr);
        inused_blocks_.erase(block_nr);
        cur_blocks_--;
    }
    space_lock_.unlock();
    return true;
}
