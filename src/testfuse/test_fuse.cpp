#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <features.h>
#include <memory>
#include <string>
#include <sys/types.h>
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


using namespace std;

mutex mtx;

static struct fuse_operations fuse_oper;
std::shared_ptr<GreeterClient> spongebobfs = nullptr;
// char den_name_list[32][32];

static int fuse_open(const char *path, struct fuse_file_info *fi)
{
	const char* new_path = path + 1;
	printf("fuse_open: %s\n", new_path);
	int res = spongebobfs->OpenFile(new_path);
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
	bool success = spongebobfs->CloseFile(fi->fh);
	if (!success) {
		return -errno;
	}
	return 0;
}

static int fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
	// cout << "readdir" << endl;
	// filler(buf, "fuse_readdir", NULL, 0);
	const char* new_path = path + 1;
	vector<struct dentry_info> dentry_list;
	int success =  spongebobfs->ReadDirectory(new_path, dentry_list);
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
	// for (int i = 0; i < dentry_list.size(); ++i) {
	// 	memset(&st, 0, sizeof(st));
	// 	// st.st_ino = d_info.inum;
	// 	st.st_mode = dentry_list[i].is_dir ? S_IFDIR: S_IFREG;
	// 	// std::cout << __func__ << ": path is " << dentry_list[i].name << std::endl;
	// 	strcpy(den_name_list[i], dentry_list[i].name.c_str());
	// 	printf("readdir: %s, path lenth: %lu\n", den_name_list[i], dentry_list[i].name.length());
	// 	if (filler(buf, den_name_list[i] + 1, &st, 0)) {
	// 		break;
	// 	}
	// }
	return 0;
}

static int fuse_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	const char* new_path = path + 1;
	int res = 0;
	std::string str = "Spongebob Read.";
	uint64_t bytes_read = 0;
	printf("%s: file path is %s, offset is %lu, read size is %lu.\n", __func__, new_path, offset, size);
	// res = nrfsRead(fs, (nrfsFile)path, buf, (uint64_t)size, (uint64_t)offset);
	auto ret_list = spongebobfs->ReadFile(new_path, offset, size, bytes_read, buf);
	std::cout << __func__ << ": Received the following block info..." << std::endl;
	for (auto cur_block : ret_list) {
		// std::cout << cur_block.get_serverid();
		std::cout << cur_block.block_idx() << ", ";
		std::cout << cur_block.serverid() << ", ";
		std::cout << cur_block.mem_offset() << ", ";
		std::cout << cur_block.length() << ", ";
		std::cout << cur_block.buff_offset() << std::endl;
	}

	// strcpy(buf, str.c_str());
	for (size_t i = 0; i < size; ++i) {
		size_t c = i % 26 + 'a';
		memcpy(buf + i, &c, sizeof(size_t));
	}
	std::cout << __func__ << ": Totally " << bytes_read << " bytes read." << std::endl;
	return bytes_read;
}

static int fuse_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	const char* new_path = path + 1;
	int res = size;
	printf("%s: file path is %s\n", __func__, new_path);
	printf("%s: write contents is %s\n", new_path, buf);

	// lock_guard<mutex> lock(mtx);
	uint64_t bytes_write = 0;
	auto ret_list = spongebobfs->WriteFile(new_path, offset, size, bytes_write, buf);
	std::cout << __func__ << ": Received the following block info..." << std::endl;
	for (auto cur_block : ret_list) {
		// std::cout << cur_block.get_serverid();
		std::cout << cur_block.block_idx() << ", ";
		std::cout << cur_block.serverid() << ", ";
		std::cout << cur_block.mem_offset() << ", ";
		std::cout << cur_block.length() << ", ";
		std::cout << cur_block.buff_offset() << std::endl;
	}
	std::cout << __func__ << ": Totally " << bytes_write << " bytes written." << std::endl;
	return bytes_write;
}

static int fuse_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	const char* new_path = path + 1;
	int res = spongebobfs->OpenFile(new_path);
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
	res = spongebobfs->RemoveFile(new_path);
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
	return 0;
}
static int fuse_rmdir(const char *path)
{
	return 0;
}
static int fuse_utimens(const char * path, const struct timespec tv[2]) {
	return 0;
}

static int fuse_getxattr(const char *path, const char *name, char *value, size_t size)
{
	return 0;
}

static int fuse_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
	return 0;
}

int main(int argc, char* argv[])
{
	spongebobfs = make_shared<GreeterClient>(grpc::CreateChannel("localhost:50055", grpc::InsecureChannelCredentials()));
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
	fuse_oper.getxattr = fuse_getxattr;
	fuse_oper.setxattr = fuse_setxattr;
	fuse_oper.fgetattr = fuse_fgetattr;
	// fs = nrfsConnect("default", 0, 0);
	fuse_main(argc, argv, &fuse_oper, NULL);
}