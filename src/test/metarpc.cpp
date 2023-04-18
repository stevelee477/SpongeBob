#include "nrfs.h"
#include <iostream>

int main() {
    nrfs fs = nrfsConnect("default", 0, 0);
    std::cout << nrfsMetaRPC(fs);
}