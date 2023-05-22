#ifndef __SF_FILE__
#define __SF_FILE__
#include <cstdint>
#include <memory>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <mutex>
#include <algorithm>
#include <iostream>


#define FILE_BLOCK_SIZE (1 << 20)
#define FILE_BLOCK_MASK (FILE_BLOCK_SIZE - 1)


enum class FileType {
    REG_FILE,
    DIR,
};

struct file_block_info {
    uint64_t server_id;
    uint64_t start_addr;
    uint64_t length;
    uint64_t block_nr;
};

class Dentry {
public:
    Dentry() = default;
    Dentry(const std::string& name, uint64_t inum);
    ~Dentry() = default;

    inline uint64_t GetHash() { return hash_; }
    inline uint64_t GetInodeNum() { return inum_; }
    inline std::string GetName() { return name_; }
    bool IsDir() { return inum_ == 0; }
private:
    std::string name_;
    uint64_t hash_;
    uint64_t inum_;
};

class File;

class Inode {
public:
    Inode() = default;
    Inode(FileType type, uint64_t inum);
    ~Inode() = default;

    uint64_t GetInodeNum() { return inum_;}
    uint64_t GetSize() { return size_; }
    uint64_t GetBlockNum() { return block_info_list_.size(); }

    file_block_info GetBlockInfo(uint64_t block_id) { return block_info_list_[block_id]; }
    const std::unordered_map<uint64_t, std::shared_ptr<Dentry>>& GetDentryMap() const { return children_; }
    std::vector<file_block_info>& GetBlockInfoList() { return block_info_list_; }
    std::shared_ptr<Dentry> GetDentry(const std::string &name);


    void ChangeFileBlockInfoLength(uint64_t index, uint64_t length);
    bool IsDir() { return type_ == FileType::DIR; }
    bool IsOpened() { return file_fd_ != -1; }
    void SetInodeNum(uint64_t inum) { inum_ = inum; }
    void SetSize(uint64_t size) { size_ = size; }
    void SetFile(int fd) { file_fd_ = fd; }
    bool AddDentry(const std::string &name, uint64_t inum);
    bool DeleteDentry(const std::string &name);
    void AppendFileBlockInfo(uint64_t server_id, uint64_t start_addr, uint64_t length, uint64_t block_nr);

private:
    uint64_t inum_;
    FileType type_;
    uint64_t size_{0};
    int file_fd_ = -1;
    std::unordered_map<uint64_t, std::shared_ptr<Dentry>> children_;
    std::vector<file_block_info> block_info_list_;
};

class InodeTable {
public:
    InodeTable();
    InodeTable(uint64_t size);
    ~InodeTable() = default;

    uint64_t AllocateFileInode() { return AllocateFreeInode(FileType::REG_FILE); }
    uint64_t AllocateDirInode() { return AllocateFreeInode(FileType::DIR); }
    uint64_t GetTotalSize() { return total_size_; }
    uint64_t GetCurSize() { return cur_size_; }
    std::shared_ptr<Inode> GetInode(uint64_t inum);
    bool DeleteInode(uint64_t inum);
    void CreateRootDir();

private:
    uint64_t total_size_;
    uint64_t cur_size_;
    mutable std::mutex table_lock_;
    std::unordered_map<uint64_t, std::shared_ptr<Inode>> table_;
    std::priority_queue<uint64_t, std::vector<uint64_t>, std::greater<uint64_t>> free_list_;

    uint64_t AllocateFreeInode(FileType type);
};

class File {
public:
    File() = default;
    File(uint64_t inum);
    ~File() = default;
    uint64_t inum;
    uint64_t file_pos;
};

class FileMap {
public:
    FileMap();
    FileMap(uint64_t size);
    ~FileMap() = default;

    int AllocateFD(uint64_t inum);
    int AllocateFD();
    // int AddFile(uint64_t fd, uint64_t inum);
    std::shared_ptr<Inode> GetFile(uint64_t fd);
    bool ReclaimFD(uint64_t fd);

private:
    uint64_t total_size_;
    mutable std::mutex file_map_lock_;
    std::shared_ptr<InodeTable> inode_table_;
    std::unordered_map<uint64_t, std::shared_ptr<File>> file_map_;
    std::unordered_set<uint64_t> fd_used_set_;
    std::priority_queue<uint64_t, std::vector<uint64_t>, std::greater<uint64_t>> fd_free_list_;
};



#endif