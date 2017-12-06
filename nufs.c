#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <bsd/string.h>
#include <assert.h>
#include <stdlib.h>


#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "storage.h"
#include "inode.h"
#include "directory.h"


int
nufs_access(const char *path, int mask)
{
    printf("access(%s, %04o)\n", path, mask);
    int result = file_exists(path);
    if (result == -1) {
      return -ENOENT;
    }
    else {
      return 0;
    }
}

int
nufs_getattr(const char *path, struct stat *st)
{
    printf("getattr(%s)\n", path);
    int rv = get_stat(path, st);
    if (rv == -1) {
        return -ENOENT;
    }
    else {
        return 0;
    }
}

int
nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
             off_t offset, struct fuse_file_info *fi)
{
    printf("readdir(%s)\n", path);
    struct stat st;
    get_stat(path, &st);

    filler(buf, ".", &st, 0);

    directory* dir = get_root_directory();
    if(dir->inum > 1) {
        dir_ent* cur_ent = (dir_ent*)(get_pointer(dir->ents_off + (64 + sizeof(dir_ent))));
        for (int i = 0; i < dir->inum - 1; i++) {
            cur_ent = (dir_ent*)(get_pointer(dir->ents_off + ((i + 1) * (64 + sizeof(dir_ent)))));

            struct stat temp;

            char* full_path = malloc(256); // used to combine two char*
            strcpy(full_path, path);
            strcat(full_path, (char*)get_pointer(cur_ent->name_off));

            get_stat(full_path, &temp);
            filler(buf, (char*)get_pointer(cur_ent->name_off), &temp, 0);
            free(full_path);
        }
    }

    return 0;
}

int
nufs_mknod(const char *path, mode_t mode, dev_t rdev)
{
    printf("mknod(%s, %04o)\n", path, mode);
    int rv = make_file(path, mode, rdev);
    if (rv == -1) {
        return -ENOENT;
    }
    else {
        return 0;
    }
}

int
nufs_mkdir(const char *path, mode_t mode)
{
    printf("mkdir(%s)\n", path);
    int rv = make_dir(path, mode);
    if (rv == -1) {
        return -ENOENT;
    }
    else {
        return 0;
    }
}

int
nufs_unlink(const char *path)
{
    printf("unlink(%s)\n", path);
    int rv = unlink_file(path);
    if (rv == -1) {
        return -ENOENT;
    }
    else {
        return 0;
    }
}

int
nufs_rmdir(const char *path)
{
    printf("rmdir(%s)\n", path);
    return -1;
}

int
nufs_rename(const char *from, const char *to)
{
    printf("rename(%s => %s)\n", from, to);
    int rv = rename_file(from, to);
    if (rv == -1) {
        return -ENOENT;
    }
    else {
        return 0;
    }
}

int
nufs_chmod(const char *path, mode_t mode)
{
    printf("chmod(%s, %04o)\n", path, mode);

    dir_ent* ent = get_file_data(path);
    inode* node = (inode*)get_pointer(ent->node_off);
    node->mode = mode;

    return 0;
}

int
nufs_truncate(const char *path, off_t size)
{
    printf("truncate(%s, %ld bytes)\n", path, size);
    return 0;
}

int
nufs_open(const char *path, struct fuse_file_info *fi)
{
    printf("open(%s)\n", path);
    return 0;
}

int
nufs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printf("read(%s, %ld bytes, @%ld)\n", path, size, offset);
    const char* data = get_data(path);

    int len = strlen(data) + 1;
    if (size < len) {
        len = size;
    }

    strlcpy(buf, data, len);
    return len;
}

int
nufs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printf("write(%s, buf: %s, %ld bytes, @%ld)\n", path, buf, size, offset);
    int rv = write_file(path, buf, size, offset, fi);
    return (int)size;
}

int
nufs_utimens(const char* path, const struct timespec ts[2])
{
    int rv = -1;
    printf("utimens(%s, [%ld, %ld; %ld %ld]) -> %d\n",
           path, ts[0].tv_sec, ts[0].tv_nsec, ts[1].tv_sec, ts[1].tv_nsec, rv);
	return rv;
}

void
nufs_init_ops(struct fuse_operations* ops)
{
    memset(ops, 0, sizeof(struct fuse_operations));
    ops->access   = nufs_access;
    ops->getattr  = nufs_getattr;
    ops->readdir  = nufs_readdir;
    ops->mknod    = nufs_mknod;
    ops->mkdir    = nufs_mkdir;
    ops->unlink   = nufs_unlink;
    ops->rmdir    = nufs_rmdir;
    ops->rename   = nufs_rename;
    ops->chmod    = nufs_chmod;
    ops->truncate = nufs_truncate;
    ops->open	  = nufs_open;
    ops->read     = nufs_read;
    ops->write    = nufs_write;
    ops->utimens  = nufs_utimens;
};

struct fuse_operations nufs_ops;

int
main(int argc, char *argv[])
{
    assert(argc > 2 && argc < 6);
    storage_init(argv[--argc]);
    nufs_init_ops(&nufs_ops);
    return fuse_main(argc, argv, &nufs_ops, NULL);
}
