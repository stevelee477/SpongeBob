#ifndef __SF_FILE__
#define __SF_FILE__
#include <cstdint>
#include <memory>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <vector>
#include <queue>
#include <mutex>
#include <algorithm>
#include <iostream>
enum class FileType {
    REG_FILE,
    DIR,
};

struct file_data_info {
    uint64_t server_id;
    uint64_t start_addr;
    uint64_t length;
};

class Dentry;

class Inode {
public:
    Inode() = default;
    Inode(std::string name, FileType type, uint64_t inum);
    ~Inode() = default;

    uint64_t GetInodeNum() { return inum_;}
    bool IsDir() { return type_ == FileType::DIR; }
    void SetInodeNum(uint64_t inum) { inum_ = inum; }
    bool AddDentry(const std::string& name, uint64_t inum);

private:
    uint64_t inum_;
    FileType type_;
    uint64_t size_{0};
    std::unordered_map<uint64_t, std::shared_ptr<Dentry>> children_;
    std::vector<file_data_info> data_info_;
};

class Dentry {
public:
    Dentry() = default;
    Dentry(const std::string& name, uint64_t inum);
    ~Dentry() = default;
private:
    std::string name;
    uint64_t hash;
    uint64_t inum;
};

class InodeTable {
public:
    InodeTable();
    InodeTable(uint64_t size);
    ~InodeTable() = default;


    bool DeleteInode(uint64_t inum);
    uint64_t AllocateFileInode() { return AllocateFreeInode(FileType::REG_FILE); }
    uint64_t AllocateDirInode() { return AllocateFreeInode(FileType::DIR); }

private:
    uint64_t total_size_;
    uint64_t cur_size_;
    mutable std::mutex table_lock_;
    std::unordered_map<uint64_t, std::shared_ptr<Inode>> table_;
    std::priority_queue<uint64_t, std::vector<uint64_t>, std::greater<uint64_t>> free_list_;

    uint64_t AllocateFreeInode(FileType type);
};




#endif