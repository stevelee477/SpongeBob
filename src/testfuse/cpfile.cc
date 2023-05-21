#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>

constexpr size_t BUFFER_SIZE = 32;
char buffer[BUFFER_SIZE];
int main() {

    int srcFile = open("cpfile.cc", O_RDONLY);
    if(srcFile == -1) {
        std::cerr << "Unable to open source file" << std::endl;
        return 1;
    }

    int destFile = open("/mnt/nvme/ligch/spongebob/build/sbfs/cp.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(destFile == -1) {
        std::cerr << "Unable to open destination file" << std::endl;
        return 1;
    }

    ssize_t bytesRead;
    int total_bytes_write = 0;
    while((bytesRead = read(srcFile, buffer, BUFFER_SIZE)) > 0) {
        if(write(destFile, buffer, bytesRead) != bytesRead) {
            std::cerr << "Error writing to destination file" << std::endl;
            return 1;
        }
        total_bytes_write += bytesRead;
    }
    std::cout << "total_bytes_write: " << total_bytes_write << std::endl;
    if(bytesRead == -1) {
        std::cerr << "Error reading from source file" << std::endl;
        return 1;
    }
    lseek(destFile, 0, SEEK_SET);
    while((bytesRead = read(destFile, buffer, BUFFER_SIZE)) > 0) {
        write(STDOUT_FILENO, buffer, bytesRead);
    }

    close(srcFile);
    close(destFile);

    return 0;
}
