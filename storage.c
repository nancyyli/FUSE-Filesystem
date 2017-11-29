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
//#include "directory.h"

const int NUFS_SIZE  = 1024 * 1024;
const int BLOCK_SIZE = 4096;

static super_block* s_block = 0;

typedef struct dirent {
  //int    pnum;
  char   name[48];
  inode* node;
} dirent;

typedef struct directory {
    int     inum;
    dirent* ents;
    inode*  node;
} directory;

typedef struct file_data {
    const char* path;
    inode* node;
} file_data;

static file_data file_table[30]; // change when the number of data blocks is calculated.

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
    s_block->inode_bitmap = (void*)s_block + sizeof(super_block);
    s_block->data_bitmap_size = 30;
    s_block->data_bitmap = s_block->inode_bitmap + s_block->inode_bitmap_size;
    s_block->inode_num = 2;
    s_block->inodes = s_block->data_bitmap + s_block->data_bitmap_size;
    s_block->data_num = 30;
    s_block->data_addr = s_block->inode_addr + (2 * sizeof(inode));
    s_block->root_node = inodes[0];
    inode* node = s_block->root_node;
    node->mode = 0744;
    node->uid = getuid();
    node->size = BLOCK_SIZE;
    node->atime = time();
    node->ctime = time();
    node->mtime = time();
    node->gid = getgid();
    node->links_count = 0;
    node->blocks = 1;
    node->flags = 0;
    node->blocks[0] = s_block->data_addr;

    directory* dir = node->blocks[0];
    dir->

    set_bit(s_block->inode_bitmap, s_block->inode_bitmap_size, 1, 0);
        printf("size of inode: %d", sizeof(inode));
        printf("size of superblock: %d", sizeof(super_block));
    //implement index of root node for s_block
}

//TODO: if file name already exists
void
make_file(const char *path, mode_t mode, dev_t rdev) {
    printf("making file: %s, %04o, %04o\n", path, mode, rdev);
    int index = get_bit_index(s_block->inode_bitmap, s_block->inode_bitmap_size);
    set_bit(s_block->inode_bitmap, s_block->inode_bitmap_size, 1, index);
    inode* node = s_block->inodes[index];
    node->mode = mode;
    node->uid = getuid();
    node->size = 0;
    node->atime = time();
    node->ctime = time();
    node->mtime = time();
    node->gid = getgid();
    node->links_count = 0;
    node->blocks = 0;
    node->flags = 1; //TODO



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
file_exists(const char* path) {
    file_data* dat = get_file_data(path);
    if (!dat) {
      return -1;
    }
    else {
      return 0;
    }
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
    return (char*)dat->node->blocks[0];
}
