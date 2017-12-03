#ifndef NUFS_STORAGE_H
#define NUFS_STORAGE_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "directory.h"

void storage_init(const char* path);
int get_stat(const char* path, struct stat* st);
int file_exists(const char* path);
const char* get_data(const char* path);
int make_file(const char *path, mode_t mode, dev_t rdev);
int write_file(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
dir_ent* get_file_data(const char* path);
void* get_pointer(int offset);
directory* get_root_directory();

#endif
