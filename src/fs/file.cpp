#include "file.hpp"
#include "debug.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <numeric>
#include <sys/types.h>

/* Dentry*/
Dentry::Dentry(const std::string& name, uint64_t inum): name_(name), inum_(inum){
    hash_ = std::hash<std::string>{}(name); // todo: find a string hash function.

}



/* Inode */
Inode::Inode(FileType type, uint64_t inum): inum_(inum), type_(type) {}
bool Inode::AddDentry(const std::string &name, uint64_t inum) {
    uint64_t hash = std::hash<std::string>{}(name); // todo: find a string hash function.
    if (children_.find(hash) == children_.end()) {
        std::cerr << " name already exists. " << std::endl;
        return false;
    }
    auto cur_dentry = std::make_shared<Dentry>(name, inum);
    children_[hash] = cur_dentry;
    return true;
}

/* Inode Table*/
InodeTable::InodeTable(): InodeTable(100) {}

InodeTable::InodeTable(uint64_t size): total_size_(size), cur_size_(0) {
    std::vector<uint64_t> vec(total_size_);
    std::iota(vec.begin(), vec.end(), 0);
    // for (uint64_t i = 0; i < total_size_; ++i) {
    //     vec[i] = i;
    // }
    free_list_ = std::priority_queue<uint64_t, std::vector<uint64_t>, std::greater<uint64_t>>(vec.begin(), vec.end());
}

std::shared_ptr<Inode> InodeTable::GetInode(uint64_t inum) {
    table_lock_.lock();
    if (table_.find(inum) == table_.end()) {
        std::cerr << __func__ << ": inode " << inum << "doesn't exist." << std::endl;
        return nullptr;
    }

    table_lock_.unlock();
    return table_[inum];
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

    auto new_inode = std::make_shared<Inode>(type, new_inum);
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
