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

FileMap::FileMap(): total_size_(100) {}

FileMap::FileMap(uint64_t size): total_size_(size) {
    std::vector<uint64_t> vec(total_size_ + 3);
    std::iota(vec.begin() + 3, vec.end(), 1);
    fd_free_list_ = std::priority_queue<uint64_t, std::vector<uint64_t>, std::greater<uint64_t>>(vec.begin(), vec.end());
    std::cout << __func__ << ": " << total_size_ << " fds in fd list." << std::endl;
}


int FileMap::AllocateFD() {
    file_map_lock_.lock();
    if (fd_free_list_.empty()) {
        std::cerr << "No free fd." << std::endl;
        file_map_lock_.unlock();
        return -1;
    }
    uint64_t new_fd = fd_free_list_.top();
    fd_free_list_.pop();
    return new_fd;
}

bool FileMap::ReclaimFD(uint64_t fd) {
    file_map_lock_.lock();
    if (fd_used_set_.find(fd) == fd_used_set_.end()) {
        std::cerr << "The fd: " << fd << " isn't in the fd list." << std::endl;
        file_map_lock_.unlock();
        return false;
    }
    fd_used_set_.erase(fd);
    fd_free_list_.push(fd);
    file_map_lock_.unlock();
    return true;
}


// int FileMap::AllocateFD(uint64_t inum) {
//     std::shared_ptr<Inode> inode = inode_table_->GetInode(inum);
//     if (inode == nullptr) {
//         std::cerr << __func__ << ": inode not exits." << std::endl;
//         return -1;
//     }

//     if (inode->IsOpened()) {
//        // todo(ligch): finish
//     }

//     if (fd_free_list_.empty()) {
//         std::cerr << "No free fd." << std::endl;
//         file_map_lock_.unlock();
//         return -1;
//     }


//     uint64_t new_fd = fd_free_list_.top();
//     fd_free_list_.pop();

//     auto new_file = std::make_shared<File>(inum);

//     file_map_[new_fd] = new_file;
//     inode->SetFile(new_fd);
//     new_file->inum = inum;
//     new_file->file_pos = 0;

//     return new_fd;
// }


// bool FileMap::ReclaimFD(uint64_t fd) {
//     file_map_lock_.lock();

//     if (file_map_.find(fd) == file_map_.end()) {
//         file_map_lock_.unlock();
//         return false;
//     }

//     uint64_t inum = file_map_[fd]->inum;
//     std::shared_ptr<Inode> inode = inode_table_->GetInode(inum);
//     if (inode == nullptr) {
//         std::cerr << __func__ << ": inode not exits." << std::endl;
//         file_map_lock_.unlock();
//         return -1;
//     }
//     inode->SetFile(-1);
//     file_map_.erase(fd);
//     fd_free_list_.push(fd);
//     file_map_lock_.unlock();
//     return true;

// }