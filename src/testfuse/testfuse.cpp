#include <cstddef>
#include <cstdio>
#include <memory>
#define FUSE_USE_VERSION 39
#include <fuse_lowlevel.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <fuse.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <mutex>
#include "MetaClient.hpp"


using namespace std;

mutex mtx;

static struct fuse_operations fuse_oper;
// std::shared_ptr<GreeterClient> spongebobfs;

static int fuse_open(const char *path, struct fuse_file_info *fi)
{
	(void) fi;

	return 0;
}

static int fuse_getattr(const char *path, struct stat *stbuf, fuse_file_info* fi)
{
	(void) fi;
	int res = 0;
	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = (1ULL << 32);
	}

	return res;
}

static int fuse_release(const char *path, struct fuse_file_info *fi)
{
	return 0;
}

static int fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{
	// cout << "readdir" << endl;
	// filler(buf, "fuse_readdir", NULL, 0);
	return 0;
}

static int fuse_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	int res = 0;
	std::string str = "Spongebob Read.";
	printf("fuse_read\n");
	// res = nrfsRead(fs, (nrfsFile)path, buf, (uint64_t)size, (uint64_t)offset);
	// res = spongebobfs->ReadFile(path, offset, size);
	// strcpy(buf, str.c_str());
	for (size_t i = 0; i < size; ++i) {
		size_t c = i % 26 + 'a';
		memcpy(buf + i, &c, sizeof(size_t));
	}
	return res;
}

static int fuse_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	int res = size;
	printf("fuse_write: %s\n", buf);

	// lock_guard<mutex> lock(mtx);
	// res = nrfsWrite(fs, (nrfsFile)path, buf, (uint64_t)size, (uint64_t)offset);
	// res = spongebobfs->WriteFile(path, offset, size);
	return res;
}

static int fuse_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	return 0;
}


static int fuse_mknod(const char *path, mode_t mode, dev_t rdev)
{
	return 0;
}


int main(int argc, char* argv[])
{
	fuse_oper.getattr = fuse_getattr;
	fuse_oper.readdir = fuse_readdir;
	fuse_oper.read = fuse_read;
	fuse_oper.write = fuse_write;
	fuse_oper.open = fuse_open;
	fuse_oper.release = fuse_release;
	fuse_oper.mknod = fuse_mknod;
	fuse_oper.create = fuse_create;
	// spongebobfs = make_shared<GreeterClient>(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
	// fs = nrfsConnect("default", 0, 0);
	fuse_main(argc, argv, &fuse_oper, NULL);
}