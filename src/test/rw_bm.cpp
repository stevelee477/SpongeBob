#include <iostream>
#include <chrono>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define FILE_SIZE_MB 100
#define FILE_NAME "testFile.bin"
#define MEGABYTE 1024 * 1024

using namespace std;
using namespace chrono;

void clearPageCache() {
    cout << "Clearing Page Cache...\n";
    if (system("sync; echo 3 > /proc/sys/vm/drop_caches") == -1) {
        cout << "Failed to clear Page Cache. Are you running as root?\n";
    }
}

void writeTest() {
    char *buffer = new char[MEGABYTE];

    // Open file
    int fd = open(FILE_NAME, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if(fd == -1) {
        cout << "Cannot open file for write.\n";
        delete[] buffer;
        return;
    }

    auto start = high_resolution_clock::now();
    for(int i = 0; i < FILE_SIZE_MB; ++i) {
        write(fd, buffer, MEGABYTE);
    }
    auto stop = high_resolution_clock::now();

    close(fd);
    delete[] buffer;

    auto duration = duration_cast<milliseconds>(stop - start);
    cout << "Write speed: " << (FILE_SIZE_MB * 1000.0 / duration.count()) << " MB/s\n";
}

void readTest() {
    char *buffer = new char[MEGABYTE];

    clearPageCache();

    // Open file
    int fd = open(FILE_NAME, O_RDONLY);
    if(fd == -1) {
        cout << "Cannot open file for read.\n";
        delete[] buffer;
        return;
    }

    auto start = high_resolution_clock::now();
    while(read(fd, buffer, MEGABYTE) > 0);
    auto stop = high_resolution_clock::now();

    close(fd);
    delete[] buffer;

    auto duration = duration_cast<milliseconds>(stop - start);
    cout << "Read speed: " << (FILE_SIZE_MB * 1000.0 / duration.count()) << " MB/s\n";
}

int main() {
    writeTest();

    readTest();
    std::remove(FILE_NAME);
    return 0;
}
