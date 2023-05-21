#include <dirent.h>
#include <iostream>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>

int main() {
    std::string filename = "/mnt/nvme/ligch/spongebob/build/sbfs/test.txt";
    // std::string filename = "./CMakeLists.txt";
    int fd = open(filename.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        std::cerr << "can't open" << std::endl;
        return 1;
    }

    std::string content = "Hello, World!\n";

    ssize_t bytes_written = write(fd, content.c_str(), content.size());
    std::cout << "bytes_written: " << bytes_written << std::endl;
    if (bytes_written == -1) {
        std::cerr << "write error" << std::endl;
        close(fd);
        return 1;
    }
    close(fd);

    fd = open(filename.c_str(), O_RDONLY);
    if (fd == -1) {
        std::cerr << "open error" << std::endl;
        return 1;
    }

    const int BUFFER_SIZE = 256;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(fd, buffer, bytes_written + 1);
    std::cout << "bytes_read: " << bytes_read << std::endl;
    // ssize_t bytes_pread = pread(fd, buffer, BUFFER_SIZE, 3);
    std::cout.write(buffer, bytes_read);
    // ssize_t bytes_read;
    // while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
    //     std::cout.write(buffer, bytes_read);
    // }
    // struct dirent *entry;
    // DIR *dir = opendir("/mnt/nvme/ligch/spongebob/build/sbfs");

    // if (dir == NULL) {
    //     std::cerr << "dir open error" << std::endl;
    //     return 1;
    // }
    // while ((entry = readdir(dir)) != NULL) {
    //     std::cout << entry->d_name << std::endl;
    // }
    // closedir(dir);


    close(fd);
    return 0;
}