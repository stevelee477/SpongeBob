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
std::shared_ptr<GreeterClient> spongebobfs = nullptr;

static int fuse_open(const char *path, struct fuse_file_info *fi)
{
	printf("fuse_open: %s\n", path);
	int res = spongebobfs->OpenFile(path);
	if (res == -1) {
		return -errno;
	}
	fi->fh = res;
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
	bool success = spongebobfs->CloseFile(fi->fh);
	if (!success) {
		return -errno;
	}
	return 0;
}

static int fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{
	// cout << "readdir" << endl;
	// filler(buf, "fuse_readdir", NULL, 0);
	vector<struct dentry_info> dentry_list;
	for (auto d_info: dentry_list) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = d_info.inum;
		st.st_mode = d_info.is_dir ? S_IFDIR | 0755 : S_IFREG | 0444;

		if (filler(buf, d_info.name.c_str(), &st, 0, (fuse_fill_dir_flags)0)) {
			break;
		}
	}
	return 0;
}

static int fuse_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	int res = 0;
	std::string str = "Spongebob Read.";
	printf("%s: file path is %s.\n", __func__, path);
	// res = nrfsRead(fs, (nrfsFile)path, buf, (uint64_t)size, (uint64_t)offset);
	res = spongebobfs->ReadFile(path, offset, size, buf);
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
	printf("%s: file path is %s\n", __func__, path);
	printf("%s: write contents is %s\n", path, buf);

	// lock_guard<mutex> lock(mtx);
	// res = nrfsWrite(fs, (nrfsFile)path, buf, (uint64_t)size, (uint64_t)offset);
	res = spongebobfs->WriteFile(path, offset, size, buf);
	return res;
}

static int fuse_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	int res = spongebobfs->OpenFile(path);
	if (res == -1) {
		return -errno;
	}
	fi->fh = res;
	return 0;
}

static int fuse_unlink(const char *path)
{
	bool res;
	res = spongebobfs->RemoveFile(path);
	if (!res) {
		return -errno;
	}
	return 0;
}

static int fuse_mknod(const char *path, mode_t mode, dev_t rdev)
{
	return 0;
}


int main(int argc, char* argv[])
{
	spongebobfs = make_shared<GreeterClient>(grpc::CreateChannel("localhost:50055", grpc::InsecureChannelCredentials()));

	fuse_oper.getattr = fuse_getattr;
	fuse_oper.readdir = fuse_readdir;
	fuse_oper.read = fuse_read;
	fuse_oper.write = fuse_write;
	fuse_oper.open = fuse_open;
	fuse_oper.release = fuse_release;
	fuse_oper.mknod = fuse_mknod;
	fuse_oper.create = fuse_create;
	fuse_oper.unlink = fuse_unlink;

	// fs = nrfsConnect("default", 0, 0);
	fuse_main(argc, argv, &fuse_oper, NULL);
}