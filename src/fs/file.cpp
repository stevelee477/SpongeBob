#include "file.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <sys/types.h>

/* Inode */
Inode::Inode(std::string name, FileType type, uint64_t inum): inum_(inum), type_(type) {}
bool Inode::AddDentry(const std::string &name, uint64_t inum) {
    uint64_t hash = inum; // todo: find a string hash function.
    if (children_.find(hash) == children_.end()) {
        std::cerr << " name already exists. " << std::endl;
        return false;
    }
    auto cur_dentry = make_shared<Dentry>(name, inum);
    children_[hash] = cur_dentry;
}

/* Inode Table*/
InodeTable::InodeTable(): InodeTable(100) {}

InodeTable::InodeTable(uint64_t size): total_size_(size), cur_size_(0) {
    for (uint64_t i = 0; i < total_size_; ++i) {
        free_list_.push(i);
    }
}

uint64_t InodeTable::AllocateFreeInode(FileType type) {
    table_lock_.lock();
    if (cur_size_ == total_size_) {
        std::cerr << "No free inode." << std::endl;
        table_lock_.unlock();
        return total_size_ + 1;
    }

    uint64_t new_inum = free_list_.top();
    free_list_.pop();

    cur_size_++;

    auto new_inode = std::make_shared<Inode>(new_inum, type);
    table_[new_inum] = new_inode;
    table_lock_.unlock();
    return new_inum;
}

bool InodeTable::DeleteInode(uint64_t inum) {
    table_lock_.lock();

    if (table_.find(inum) == table_.end()) {
        std::cerr << "The inode: " << inum << " isn't in the inode table." << std::endl;
        table_lock_.unlock();
        return false;
    }

    table_.erase(inum);
    free_list_.push(inum);
    table_lock_.unlock();
    return true;
}
