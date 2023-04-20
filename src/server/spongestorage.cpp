#include <iostream>
#include <sys/wait.h>
#include <sys/types.h>

#include "StorageServer.hpp"

std::unique_ptr<StorageServer> storageserver;

/* Catch ctrl-c and destruct. */
void Stop (int signo) {
    storageserver.release();
    _exit(0);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, Stop);
    std::cout << "Hello, world!" << std::endl;
    storageserver = std::make_unique<StorageServer>("/dev/dax1.0", 10 * 1024 * 1024);
    while (true) {
        getchar();
    }
    return 0;
}