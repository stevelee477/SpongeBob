#include <iostream>
#include <sys/wait.h>
#include <sys/types.h>
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

#include "StorageServer.hpp"
#include "debug.hpp"

std::unique_ptr<StorageServer> storageserver;

ABSL_FLAG(std::string, dax_dev, "/dev/dax0.0", "Dax device file");
ABSL_FLAG(int32_t, dax_size, 10, "Dax device mapping size");

/* Catch ctrl-c and destruct. */
void Stop (int signo) {
    storageserver.release();
    _exit(0);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, Stop);
    absl::ParseCommandLine(argc, argv);
    storageserver = std::make_unique<StorageServer>(absl::GetFlag(FLAGS_dax_dev).c_str(), absl::GetFlag(FLAGS_dax_size) * 1024 * 1024);
    while (true) {
        getchar();
    }
    return 0;
}