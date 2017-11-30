#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

#include "storage.h"
#include "super_block.h"
#include "inode.h"
#include "slist.h"
#include "util.h"

//#include "directory.h"

const int NUFS_SIZE  = 1024 * 1024;
const int BLOCK_SIZE = 4096;

static super_block* s_block = 0;

//TODO: move this to its own file
typedef struct dirent {
  //int    pnum;
  const char*  name;
  inode* node;
} dirent;

typedef struct directory {
    int     inum; // number of entries in directory
    dirent* ents;
    inode*  node;
} directory;
/*
typedef struct file_data {
    const char* path;
    inode* node;
} file_data;
*/
//static file_data file_table[30]; // change when the number of data blocks is calculated.

/*
    Run through the bits until a 0 is found.
    returns -1 if not 0 bit is found.
*/
int
get_bit_index(char* bits, int size)
{
    int count = 0;
    char temp = bits[0]; // maybe, might have to look into it.

    for (int i = 0; i < size; i++) {

        if (count >= 8) {
            count = 0;
            temp = bits[i + 1];
        }
        else {
            if (!(((temp >> count) & 1) ^ 0)) {
                return count + (i * 8);
            }
            i--;
        }
    }

    return -1;
}

/*
    sets the indecated bit to a certain value.
*/
void
set_bit(char* bits, int size, int val, int index)
{
    // find the largest multiple of 8. this will lead to the required char.
    // then use the rest of index to find the specific bit to be set.
    //
    // (char that contains the bit) |= 1 << (rest of index)
    // does this just set a bit to one?

    int char_index = index / 8;
    int bit_index = index % 8;
    //index = first_part + second_part;

    char temp = bits[char_index];

    if (val) {
        // set bit
        temp |= 1 << bit_index;
    }
    else {
        // unset bit
        temp &= ~(1 << bit_index);
    }
}

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

    printf("pointer of s_block %p\n", (void*)s_block);
    s_block->inode_bitmap_size = 2;
    s_block->inode_bitmap = (void*)s_block + sizeof(super_block);
    printf("inode_bitmap: %p\n", (void*) s_block->inode_bitmap);
    s_block->data_bitmap_size = 30;
    s_block->data_bitmap = s_block->inode_bitmap + s_block->inode_bitmap_size;
    s_block->inode_num = 2;
    s_block->inodes = (inode*)s_block->data_bitmap + s_block->data_bitmap_size;
    s_block->data_num = 30;
    s_block->data_addr = s_block->inodes + (s_block->inode_num * sizeof(inode));
    s_block->root_node = s_block->inodes;
    printf("Made s_block\n");

    inode* r_node = s_block->root_node;
    printf("r_node: %p\n", (void*) r_node);
    r_node->mode = 040755;
    r_node->uid = getuid();
    r_node->size = BLOCK_SIZE;
    r_node->atime = time(NULL);
    r_node->ctime = time(NULL);
    r_node->mtime = time(NULL);
    r_node->gid = getgid();
    r_node->links_count = 0;
    r_node->block = 1;
    r_node->flags = 0;
    r_node->blocks[0] = s_block->data_addr;
    printf("made r_node\n");
    directory* dir = r_node->blocks[0];
    dir->node = r_node;
    dir->inum = 1;
    dir->ents = (void*)dir + sizeof(inode) + sizeof(int);
    dir->ents->name = ".";
    dir->ents->node = r_node;

    printf("made directory\n");
    set_bit(s_block->inode_bitmap, s_block->inode_bitmap_size, 1, 0);
    printf("Ending init\n");
    //    printf("size of inode: %d", sizeof(inode));
    //    printf("size of superblock: %d", sizeof(super_block));
    //implement index of root node for s_block
}

//TODO: if file name already exists
int
make_file(const char *path, mode_t mode, dev_t rdev) {
    printf("making file: %s, %04o\n", path, mode);
    int index = get_bit_index(s_block->inode_bitmap, s_block->inode_bitmap_size);
    set_bit(s_block->inode_bitmap, s_block->inode_bitmap_size, 1, index);
    inode* node = s_block->inodes + (index * sizeof(inode));
    node->mode = mode;
    node->uid = getuid();
    node->size = 0;
    node->atime = time(NULL);
    node->ctime = time(NULL);
    node->mtime = time(NULL);
    node->gid = getgid();
    node->links_count = 0;
    node->block = 0;
    node->flags = 1; //TODO

    directory* root = (directory*)s_block->root_node->blocks[0];

    dirent* new_ent = root->ents + root->inum;
    new_ent->node = node;
    new_ent->name = path;
    root->inum += 1;

    return 0;
}


static dirent*
get_file_data(const char* path) {
    printf("going into get_file_data\n");
    directory* root = (directory*)s_block->root_node->blocks[0];
    if (streq(path, "/")) {
        printf("getting dirent for root\n");
        return root->ents;
    }
    slist* path_list = s_split(path,  '/');

    char* current = path_list->data;
    directory* temp = root;
    printf("going into while loop in get_file_data\n");
    while (path_list != NULL) {
        for(int i = 0; i < temp->inum; i++) {
            if(streq(current, (temp->ents + i)->name)) {
                dirent* cur_ent = (temp->ents + i);

                if(path_list->next == 0) {
                    return cur_ent;
                }
                else {
                    // check if the dirent is a file or directory.
                    if(cur_ent->node->flags == 0) {
                        path_list = path_list->next;
                        temp = (directory*) cur_ent->node->blocks[0];
                    }
                    else {
                        // maybe change for an error
                        printf("%s is an invalid path\n", path);
                        return 0;
                    }
                }
            }
        }
        printf("path_list = path_list->next\n");
        path_list = path_list->next;
    }
  /*  for (int ii = 0; 1; ++ii) {
        file_data row = file_table[ii];

        if (file_table[ii].path == 0) {
            break;
        }

        if (streq(path, file_table[ii].path)) {
            return &(file_table[ii]);
        }
    }
*/
    printf("%s does not exist\n", path);
    return 0;
}

int
file_exists(const char* path) {
    dirent* dat = get_file_data(path);
    printf("file does exist");
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
    dirent* dat = get_file_data(path);
    if (!dat) {
        return -1;
    }

    memset(st, 0, sizeof(struct stat));
    st->st_uid  = getuid();
    st->st_mode = dat->node->mode;
    st->st_gid = getgid();
    st->st_nlink = dat->node->links_count;
    st->st_atime = dat->node->atime;
    st->st_mtime = dat->node->mtime;
    st->st_dev = dat->node->dev;
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
    dirent* dat = get_file_data(path);
    if (!dat) {
        return 0;
    }

    // TODO: read all of the data in the blocks into a const char* and return.
    return (char*)dat->node->blocks[0];
}
