#define FUSE_USE_VERSION 29
#include <iostream>
#include <stdio.h>
#include <string.h>
#include "fuse.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <mutex>
using namespace std;

mutex mtx;

static struct fuse_operations fuse_oper;

static int fuse_getattr(const char *path, struct stat *stbuf)
{
	cout << "getattr" << endl;
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = 0755 | S_IFDIR;
	} else {
		stbuf->st_mode = 0644 | S_IFREG;
	}

    return 0;
}

static int fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	// cout << "readdir" << endl;
	filler(buf, "fuse_readdir", NULL, 0);
	return 0;
}

int main(int argc, char* argv[])
{
	fuse_oper.getattr = fuse_getattr;
	fuse_oper.readdir = fuse_readdir;
	// fs = nrfsConnect("default", 0, 0);
	fuse_main(argc, argv, &fuse_oper, NULL);
}