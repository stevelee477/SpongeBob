#include "pool.hpp"

Pool::Pool(std::string dev, size_t _size) : size(_size) {
    dev_fd = open(dev.c_str(), O_RDWR);
    if (dev_fd < 0) {
        std::perror("open");
    }
    void *mmptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, 0);
    if (mmptr == MAP_FAILED) {
        std::perror("mmap");
    }
    MemoryBaseAddress = reinterpret_cast<uint64_t>(mmptr);
}

Pool::~Pool() {
    munmap(reinterpret_cast<void *>(MemoryBaseAddress), size);
    close(dev_fd);
}