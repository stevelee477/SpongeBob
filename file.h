#include <cstdint>
#include <string>
#include <vector>

enum class FileType {
    REG_FILE,
    DIR,
};

struct file_data_info {
    uint64_t server_id;
    uint64_t start_addr;
    uint64_t length;
};

class Inode {
public:
    Inode();
    Inode(std::string name, FileType type, uint64_t hash, uint64_t ino);
    ~Inode();
private:
    std::string _name;
    FileType _type;
    uint64_t _hash;
    uint64_t _ino;
    uint64_t _size{0};
    std::vector<file_data_info> _data_info;
};

Inode::Inode() {

}

Inode::Inode(std::string name, FileType type, uint64_t hash, uint64_t ino): _name(name), _type(type), _hash(hash), _ino(ino) {

}