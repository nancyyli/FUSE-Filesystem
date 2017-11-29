#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include "storage.h"
#include "super_block.h"
#include "inode.h"

const int NUFS_SIZE  = 1024 * 1024;

static super_block* s_block = 0;

typedef struct file_data {
    const char* path;
    inode* node;
} file_data;

static file_data file_table[30]; // chqange when the  umber of data blocks is calculated.

void
storage_init(const char* path)
{
    printf("Store file system data in: %s\n", path);
    int fd = open(path, O_CREAT | O_RDWR, 0644);
    assert(fd != -1);

    int rv = ftruncate(fd, NUFS_SIZE);
    assert(rv == 0);

    s_block = mmap(0, NUFS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(s_block != MAP_FAILED);
    s_block->inode_bitmap_size = 2;
    s_block->inode_bitmap_addr = (void*)s_block + sizeof(s_block);
    s_block->data_bitmap_size = 30;
    s_block->data_bitmap_addr = s_block->inode_bitmap_addr + s_block->inode_bitmap_size;
    s_block->inode_num = 2;
    s_block->inode_addr = s_block->data_bitmap_addr + s_block->data_bitmap_size;
    s_block->data_num = 30;
    s_block->data_addr = s_block->inode_addr + (2 * sizeof(inode));
    //implement index of root node for s_block
}

static int
streq(const char* aa, const char* bb)
{
    return strcmp(aa, bb) == 0;
}

static file_data*
get_file_data(const char* path) {
    for (int ii = 0; 1; ++ii) {
        file_data row = file_table[ii];

        if (file_table[ii].path == 0) {
            break;
        }

        if (streq(path, file_table[ii].path)) {
            return &(file_table[ii]);
        }
    }

    return 0;
}

int
get_stat(const char* path, struct stat* st)
{
    file_data* dat = get_file_data(path);
    if (!dat) {
        return -1;
    }

    memset(st, 0, sizeof(struct stat));
    st->st_uid  = getuid();
    st->st_mode = dat->node->mode;
    st->st_gid = getuid();
    st->st_nlink = dat->node->links_count;
    st->st_atime = dat->node->atime;
    st->st_mtime = dat->node->mtime;
    if (dat->node) {
        st->st_size = dat->node->size;
    }
    else {
        st->st_size = 0;
    }
    return 0;
}

const char*
get_data(const char* path)
{
    file_data* dat = get_file_data(path);
    if (!dat) {
        return 0;
    }

    // TODO: read all of the data in the blocks into a const char* and return.
    return dat->node->block;
}
