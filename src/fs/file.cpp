#include "file.hpp"
#include "debug.hpp"
#include <cassert>
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
    if (children_.find(hash) != children_.end()) {
        std::cerr << " name already exists. " << std::endl;
        return false;
    }
    auto cur_dentry = std::make_shared<Dentry>(name, inum);
    children_[hash] = cur_dentry;
    return true;
}

std::shared_ptr<Dentry> Inode::GetDentry(const std::string &name) {
    uint64_t hash = std::hash<std::string>{}(name);
    if (children_.find(hash) == children_.end()) {
        std::cerr << " name doesn't exist. " << std::endl;
        return nullptr;
    }
    return children_[hash];
}

void Inode::ChangeFileBlockInfoLength(uint64_t index, uint64_t length) {
    assert(index < block_info_list_.size());
    block_info_list_[index].length = length;
}

void Inode::AppendFileBlockInfo(uint64_t server_id, uint64_t start_addr, uint64_t length) {
    block_info_list_.push_back({server_id, start_addr, length});
}

/* Inode Table*/
InodeTable::InodeTable(): InodeTable(100) {}

InodeTable::InodeTable(uint64_t size): total_size_(size), cur_size_(1) {
    std::vector<uint64_t> vec(total_size_);
    std::iota(vec.begin(), vec.end(), 1);
    // for (uint64_t i = 0; i < total_size_; ++i) {
    //     vec[i] = i;
    // }
    free_list_ = std::priority_queue<uint64_t, std::vector<uint64_t>, std::greater<uint64_t>>(vec.begin(), vec.end());
    std::cout << __func__ << ": " << total_size_ << " inodes in inode table." << std::endl;
    CreateRootDir();
}

std::shared_ptr<Inode> InodeTable::GetInode(uint64_t inum) {
    table_lock_.lock();
    if (table_.find(inum) == table_.end()) {
        std::cerr << __func__ << ": inode " << inum << " doesn't exist." << std::endl;
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
    cur_size_--;
    table_lock_.unlock();
    return true;
}

void InodeTable::CreateRootDir() {
    table_[0] = std::make_shared<Inode>(FileType::DIR, 0);
}