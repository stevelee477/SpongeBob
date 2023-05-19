#include "spacemanager.hpp"
#include <cstddef>
#include <cstdint>
#include <numeric>

SpaceManager::SpaceManager(uint64_t space_start, uint64_t space_end, uint64_t block_size, uint64_t server_id)
    :space_start_(space_start), space_end_(space_end), block_size_(block_size), cur_blocks_(0), server_id_(server_id) {
    uint64_t block_mask = block_size - 1;
    if (block_mask & space_start_) {
        std::cout << __func__ <<  ": space start or space end is not aligned with block size." << std::endl;
        space_start_ = (space_start_ + block_size_) & (~block_mask);
        std::cout << __func__ << ": change space start from " << space_start << " to " << space_start_ << std::endl;
    }
    total_bytes_ = space_end_ - space_start_ + 1;
    total_blocks_ = total_bytes_ / block_size_;
    std::vector<uint64_t> vec(total_blocks_);
    std::iota(vec.begin(), vec.end(), 0);
    // for (uint64_t i = 0; i < total_blocks_; ++i) {
    //     vec[i] = i;
    // }
    free_list_ = std::priority_queue<uint64_t, std::vector<uint64_t>, std::greater<uint64_t>>(vec.begin(), vec.end());
    std::cout << __func__ << ": space start: " << space_start_ << ", space end: " << space_end_ << std::endl;
    std::cout << __func__ << ": " << total_blocks_ << " file blocks in total." << std::endl;
}

std::vector<uint64_t> SpaceManager::AllocateSpace(uint64_t length) {
    space_lock_.lock();
    uint64_t to_allocate = (length + block_size_ - 1) / block_size_;
    if (to_allocate > total_blocks_ - cur_blocks_) {
        std::cerr << __func__ << ": Space isn't enough." << std::endl;
        space_lock_.unlock();
        return std::vector<uint64_t>();
    }
    std::cout << __func__ << ": start to allocate blocks.... " << std::endl;
    std::vector<uint64_t> res(to_allocate);
    for (uint64_t i = 0; i < to_allocate; ++i) {
        res[i] = free_list_.top();
        free_list_.pop();
        inused_blocks_.insert(res[i]);
        std::cout << res[i] << " " << std::endl;
        cur_blocks_++;
    }
    std::cout << std::endl;
    std::cout << __func__ << ": allocate " << to_allocate << " blocks." << std::endl;
    space_lock_.unlock();
    return res;
}

uint64_t SpaceManager::AllocateOneBlock() {
    std::cout << "Allocate one block @" << server_id_ << std::endl;
    space_lock_.lock();
    uint64_t block_nr = free_list_.top();
    free_list_.pop();
    inused_blocks_.insert(block_nr);
    cur_blocks_++;
    space_lock_.unlock();
    return block_nr;
}


bool SpaceManager::ReclaimInodeSpace(std::shared_ptr<Inode> inode) {
    if (inode == nullptr) {
        std::cerr << __func__  << ": inode is nullptr." << std::endl;
    }
    space_lock_.lock();
    auto block_info_list = inode->GetBlockInfoList();
    std::cout << __func__ << ": start to reclaim space from inode " << inode->GetInodeNum() << std::endl;
    uint64_t total = 0;

    for (auto block_info: block_info_list) {
        uint64_t server_id = block_info.server_id;
        // (void) server_id;
        if (server_id != server_id_)
            continue;
        uint64_t block_nr = block_info.block_nr;
        std::cout << block_nr << " ";
        free_list_.push(block_nr);
        inused_blocks_.erase(block_nr);
        cur_blocks_--;
        total++;
    }
    std::cout << __func__ << ": " << total << " blocks reclaimed." << std::endl;
    space_lock_.unlock();
    return true;
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

void SpaceManager::ResetSpaceRange(uint64_t start_addr, uint64_t length) {
    uint64_t block_mask = block_size_ - 1;
    space_start_ = start_addr;
    space_end_ = space_start_ + length - 1;
    if (block_mask & space_start_) {
        std::cerr << __func__ << ": space start or space end is not aligned with block size." << std::endl;
        space_start_ = (space_start_ + block_size_) & (~block_mask);
        std::cout << __func__ << ": change space start to " << space_start_ << std::endl;
    }
    total_bytes_ = space_end_ - space_start_ + 1;
    total_blocks_ = total_bytes_ / block_size_;
    std::vector<uint64_t> vec(total_blocks_);
    std::iota(vec.begin(), vec.end(), 0);
    // for (uint64_t i = 0; i < total_blocks_; ++i) {
    //     vec[i] = i;
    // }
    free_list_ = std::priority_queue<uint64_t, std::vector<uint64_t>, std::greater<uint64_t>>(vec.begin(), vec.end());
    std::cout << __func__ << ": space start: " << space_start_ << ", space end: " << space_end_ << std::endl;
    std::cout << __func__ << ": " << total_blocks_ << " file blocks in total." << std::endl;

}