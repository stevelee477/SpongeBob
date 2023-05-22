#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <cstring>

constexpr size_t BUFFER_SIZE = 2048;
char buffer[BUFFER_SIZE];
int main() {

    int srcFile = open("original.txt", O_RDONLY);
    if(srcFile == -1) {
        std::cerr << "Unable to open source file" << std::endl;
        return 1;
    }

    int destFile = open("./sbfs/cp.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
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
    memset(buffer, 0, BUFFER_SIZE);
    int out = open("out.txt", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    int tot_bytes_read = 0;
    while((bytesRead = read(destFile, buffer, BUFFER_SIZE)) > 0) {
        // printf("\nbytes_read: %ld\n", bytesRead);
        tot_bytes_read += bytesRead;
        // write(STDOUT_FILENO, buffer, bytesRead);
        write(out, buffer, bytesRead);
    }
    // bytesRead = read(destFile, buffer, BUFFER_SIZE);
    printf("\nbytes_read: %ld\n", tot_bytes_read);

    close(srcFile);
    close(destFile);

    return 0;
}
