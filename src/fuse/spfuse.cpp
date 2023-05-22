#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#define FUSE_USE_VERSION 26
// #include <fuse_lowlevel.h>
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
#include "Client.hpp"

using namespace std;

mutex mtx;

static struct fuse_operations fuse_oper;
std::shared_ptr<Client> spongebobfs = nullptr;
// char den_name_list[32][32];

static int fuse_open(const char *path, struct fuse_file_info *fi)
{
	const char* new_path = path + 1;
	printf("fuse_open: %s\n", new_path);
	int res = spongebobfs->getMetaClient()->OpenFile(new_path);
	if (res == -1) {
		return -errno;
	}
	fi->fh = res;
	return 0;
}

static int fuse_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;
	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_size = 0;
	} else {
		stbuf->st_mode = S_IFREG | 0755;
		stbuf->st_size = 123;
	}
	stbuf->st_nlink = 1;            /* Count of links, set default one link. */
    stbuf->st_uid = 0;              /* User ID, set default 0. */
    stbuf->st_gid = 0;              /* Group ID, set default 0. */
    stbuf->st_rdev = 0;             /* Device ID for special file, set default 0. */

	//stbuf->st_blksize = BLOCK_SIZE;

    stbuf->st_atime = 0;            /* Time of last access, set default 0. */
    stbuf->st_mtime = 0; 			/* Time of last modification, set default 0. */
    stbuf->st_ctime = 0;            /* Time of last creation or status change, set default 0. */
	return res;
}

static int fuse_release(const char *path, struct fuse_file_info *fi)
{
	bool success = spongebobfs->getMetaClient()->CloseFile(fi->fh);
	if (!success) {
		return -errno;
	}
	return 0;
}

static int fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	const char* new_path = path + 1;
	vector<struct dentry_info> dentry_list;
	int success =  spongebobfs->getMetaClient()->ReadDirectory(new_path, dentry_list);
	struct stat st;
	for (auto d_info: dentry_list) {
		memset(&st, 0, sizeof(st));
		// st.st_ino = d_info.inum;
		st.st_mode = d_info.is_dir ? S_IFDIR: S_IFREG;
		printf("readdir: %s\n", d_info.name.c_str());
		if (filler(buf, d_info.name.c_str(), &st, 0)) {
			break;
		}
	}

	return 0;
}

static int fuse_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	const char* new_path = path + 1;
	int res = 0;
	std::string str = "Spongebob Read.";
	Debug::debugItem("%s: file path is %s size is %d offset is %d", __func__, new_path, size, offset);

	auto bytes_read = spongebobfs->read(new_path, buf, offset, size);

	return bytes_read;
}

static int fuse_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	const char* new_path = path + 1;
	int res = size;
	Debug::debugItem("%s: file path is %s size is %d offset is %d", __func__, new_path, size, offset);

	auto bytes_write = spongebobfs->write(new_path, const_cast<char*>(buf), offset, size);

	return bytes_write;
}

static int fuse_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	const char* new_path = path + 1;
	int res = spongebobfs->getMetaClient()->OpenFile(new_path);
	if (res == -1) {
		return -errno;
	}
	fi->fh = res;

	return 0;
}

static int fuse_unlink(const char *path)
{
	bool res;
	const char* new_path = path + 1;
	res = spongebobfs->getMetaClient()->RemoveFile(new_path);
	if (!res) {
		return -errno;
	}
	return 0;
}

static int fuse_mknod(const char *path, mode_t mode, dev_t rdev)
{
	return 0;
}

static int fuse_chmod(const char *path, mode_t mode)
{
	return 0;
}

static int fuse_access(const char *path, int mask)
{
	int res;
	return 0;
	// res = nrfsAccess(fs, path);
}

static int fuse_flush(const char *path, struct fuse_file_info *fi)
{
	return 0;
}
static int fuse_statfs(const char *path, struct statvfs *stbuf)
{
	stbuf->f_bsize = 12345;
	return 0;
}

static int fuse_setxattr (const char *path, const char *name, const char *value, size_t size, int flags)
{
	return 0;
}
static int fuse_mkdir(const char *path, mode_t mode)
{
	bool res;
	const char* new_path = path + 1;
	res = spongebobfs->getMetaClient()->CreateDiretory(new_path);
	if (!res) {
		return -errno;
	}
	return 0;
}
static int fuse_rmdir(const char *path)
{
	bool res;
	const char* new_path = path + 1;
	res = spongebobfs->getMetaClient()->RemoveFile(new_path);
	if (!res) {
		return -errno;
	}
	return 0;
}
static int fuse_utimens(const char * path, const struct timespec tv[2]) {
	return 0;
}

int main(int argc, char* argv[])
{
	spongebobfs = make_shared<Client>();
	fuse_oper.access = fuse_access;
	fuse_oper.getattr = fuse_getattr;
	fuse_oper.readdir = fuse_readdir;
	fuse_oper.read = fuse_read;
	fuse_oper.write = fuse_write;
	fuse_oper.open = fuse_open;
	fuse_oper.release = fuse_release;
	// fuse_oper.mknod = fuse_mknod;
	fuse_oper.utimens = fuse_utimens;
	fuse_oper.create = fuse_create;
	fuse_oper.unlink = fuse_unlink;
	fuse_oper.statfs = fuse_statfs;
	fuse_oper.setxattr = fuse_setxattr;
	fuse_oper.flush = fuse_flush;
	fuse_oper.mkdir = fuse_mkdir;
	fuse_oper.rmdir = fuse_rmdir;

	// fs = nrfsConnect("default", 0, 0);
	fuse_main(argc, argv, &fuse_oper, NULL);
}