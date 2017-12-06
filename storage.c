#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "storage.h"
#include "super_block.h"
#include "inode.h"
#include "slist.h"
#include "util.h"
#include "directory.h"

const int NUFS_SIZE  = 1024 * 1024;
const int BLOCK_SIZE = 4096;
const int INODE_SIZE =  sizeof(inode) + (sizeof(int) * 15);
const int DIRENT_SIZE = sizeof(dir_ent) + 64;

static int counter = 0;
static super_block* s_block = 0;

/*
    Run through the bits until a 0 is found.
    returns -1 if not 0 bit is found.
*/
int
get_bit_index(char* bits, int size)
{
    int count = 0;
    char temp = *(bits);

    for (int i = 0; i < size; i++) {

        if (count >= 8) {
            count = 0;
            temp = *(bits + (i + 1));
        }
        else {
            if (!(((temp >> count) & 1) ^ 0)) {
                return count + (i * 8);
            }
            i--;
            count += 1;
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

    int char_index = index / 8;
    int bit_index = index % 8;

    char temp = *(bits + char_index);

    if (val) {
        // set bit
        temp |= 1 << bit_index;
    }
    else {
        // unset bit
        temp &= ~(1 << bit_index);
    }
    *(bits + char_index) = temp;
}

/*
    calculates the pointer starting at the super block, given some offset.
*/
void*
get_pointer(int offset) {
    return ((void*)s_block) + offset;
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
    if(s_block->inode_bitmap_size != 4) {
        s_block->inode_bitmap_size = 4;
        s_block->inode_bitmap_off =  sizeof(super_block);
        s_block->data_bitmap_size = 6;
        s_block->data_bitmap_off = s_block->inode_bitmap_off + s_block->inode_bitmap_size;
        s_block->inode_num = 20;
        s_block->inodes_off = s_block->data_bitmap_off + s_block->data_bitmap_size;
        s_block->data_num = 48;
        s_block->data_blocks_off = s_block->inodes_off + (s_block->inode_num * INODE_SIZE);
        s_block->root_node_off = s_block->inodes_off;

        inode* r_node = (inode*)get_pointer(s_block->root_node_off);
        r_node->mode = 040755;
        r_node->uid = getuid();
        r_node->size = BLOCK_SIZE;
        r_node->atime = time(NULL);
        r_node->ctime = time(NULL);
        r_node->mtime = time(NULL);
        r_node->gid = getgid();
        r_node->links_count = 0;
        r_node->num_blocks = 1;
        r_node->flags = 0;
        r_node->blocks_off = s_block->root_node_off + sizeof(inode);
        int* temp_pointer = (int*)get_pointer(r_node->blocks_off);
        *(temp_pointer) = s_block->data_blocks_off;

        directory* dir = (directory*)get_pointer(*(temp_pointer));
        dir->node_off = s_block->root_node_off;
        dir->inum = 1;
        dir->ents_off = s_block->data_blocks_off + sizeof(directory);
        dir_ent* root_dirent = (dir_ent*)get_pointer(dir->ents_off);
        root_dirent->name_off = dir->ents_off + sizeof(dir_ent);
        char* name = (char*)get_pointer(root_dirent->name_off);
        strcpy(name, "/");

        root_dirent->node_off = s_block->root_node_off;

        set_bit((char*)get_pointer(s_block->inode_bitmap_off), s_block->inode_bitmap_size, 1, 0);
        set_bit((char*)get_pointer(s_block->data_bitmap_off), s_block->data_bitmap_size, 1, 0);
    }
}

int
make_file(const char *path, mode_t mode, dev_t rdev) {
    printf("making file: %s, %04o\n", path, mode);
    int index = get_bit_index((char*)get_pointer(s_block->inode_bitmap_off), s_block->inode_bitmap_size);
    set_bit((char*)get_pointer(s_block->inode_bitmap_off), s_block->inode_bitmap_size, 1, index);

    inode* node = (inode*)(get_pointer(s_block->inodes_off) + (index * INODE_SIZE));
    node->mode = mode;
    node->uid = getuid();
    node->size = 0;
    node->atime = time(NULL);
    node->ctime = time(NULL);
    node->mtime = time(NULL);
    node->gid = getgid();
    node->links_count = 1;
    node->num_blocks = 0;
    node->flags = 1; // TODO
    node->blocks_off = sizeof(inode) + (s_block->inodes_off + (index * INODE_SIZE));

    int* temp_pointer = (int*)get_pointer(((inode*)get_pointer(s_block->root_node_off))->blocks_off);

    directory* root = (directory*)get_pointer(*(temp_pointer));
    dir_ent* new_ent = (dir_ent*)(get_pointer(root->ents_off + (root->inum * DIRENT_SIZE)));
    new_ent->node_off = s_block->inodes_off + (index * INODE_SIZE);
    new_ent->name_off = root->ents_off + ((root->inum + 2) * DIRENT_SIZE);
    slist* path_name = s_split(path, '/');
    char* name = (char*)get_pointer(new_ent->name_off);
    strcpy(name, path_name->next->data);
    root->inum += 1;

    return 0;
}

dir_ent*
get_file_data(const char* path) {
    int* temp_pointer = (int*)get_pointer(((inode*)get_pointer(s_block->root_node_off))->blocks_off);
    directory* root = (directory*)get_pointer(*(temp_pointer));
    if (streq(path, "/")) {
        return (dir_ent*)get_pointer(root->ents_off);
    }
    slist* path_list = s_split(path,  '/');

    path_list = path_list->next; // first is empty
    char* current = path_list->data;
    directory* temp = root;
    dir_ent* temp_ent = (dir_ent*)get_pointer(temp->ents_off + DIRENT_SIZE);
    while (path_list != NULL) {
        for(int i = 0; i < temp->inum - 1; i++) {
            if(streq(current, (char*)get_pointer(temp_ent->name_off))) {
                dir_ent* cur_ent = temp_ent;

                if(path_list->next == 0) {
                    return cur_ent;
                }
                else {
                    inode* cur_node = (inode*)get_pointer(cur_ent->node_off);

                    // check if the dirent is a file or directory.
                    if(cur_node->flags == 0) {
                        path_list = path_list->next;
                        temp_pointer = (int*)get_pointer(cur_node->blocks_off);
                        temp = (directory*)get_pointer(*(temp_pointer));
                        temp_ent = (dir_ent*)get_pointer(temp->ents_off);
                        i = 0;
                        break;
                    }
                    else {
                        printf("%s is an invalid path\n", path);
                        return 0;
                    }
                }
            }
            temp_ent = (dir_ent*)((void*)(temp_ent) + DIRENT_SIZE);
        }
        path_list = path_list->next;
    }

    printf("%s does not exist\n", path);
    return 0;
}

int
write_file(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int off = offset % BLOCK_SIZE;
    int block_num = offset / BLOCK_SIZE;

    dir_ent* dat = get_file_data(path);
    if (!dat || size >= BLOCK_SIZE) {
        return -1;
    }

    inode* node = (inode*)get_pointer(dat->node_off);

    int* temp_pointer = 0;

    // add mem blocks to the node if necessary
    if((offset + size)/BLOCK_SIZE >= node->num_blocks) {
        for(int i = 0; i < 1; i++) {
            // get index from bitmap
            int index = get_bit_index((char*)get_pointer(s_block->data_bitmap_off), s_block->data_bitmap_size);
            set_bit((char*)get_pointer(s_block->data_bitmap_off), s_block->data_bitmap_size, 1, index);

            // set the block offeset in the node
            temp_pointer = ((int*)get_pointer(node->blocks_off)) + node->num_blocks;
            *(temp_pointer) = (index * BLOCK_SIZE) + s_block->data_blocks_off;
            node->num_blocks += 1;
        }
    }
    temp_pointer = (int*)get_pointer(node->blocks_off);
    int* roottemp = (int*)get_pointer(s_block->data_blocks_off);

    // get the data from the block we are writting to
    temp_pointer = ((int*)get_pointer(node->blocks_off)) + block_num;
    char* file_data =  (char*)get_pointer(*(temp_pointer));

    // write the data
    for (int i = 0; i < size; i++) {
        *(file_data + off) = *(buf + i);
        off += 1;
    }
    temp_pointer = (int*)get_pointer(node->blocks_off);

    node->size = node->size - offset + size;
    node->mtime = time(NULL);

    return 0;
}

int
file_exists(const char* path)
{
    dir_ent* dat = get_file_data(path);

    if (!dat) {
        printf("file doesnt exist\n");
        return -1;
    }
    else {
        printf("file does exist\n");
        return 0;
    }
}

directory*
get_root_directory()
{
    inode* r_inode = (inode*)get_pointer(s_block->root_node_off);
    directory* dir = (directory*)get_pointer(s_block->data_blocks_off);
    return dir;
}

int
get_stat(const char* path, struct stat* st)
{
    dir_ent* dat = get_file_data(path);
    if (!dat) {
        return -1;
    }

    inode* cur_node = (inode*)get_pointer(dat->node_off);

    memset(st, 0, sizeof(struct stat));
    st->st_uid  = getuid();
    st->st_gid = getgid();
    st->st_nlink = cur_node->links_count;
    st->st_atime = cur_node->atime;
    st->st_mtime = cur_node->mtime;
    st->st_dev = cur_node->dev;
    if (cur_node->flags == 1) {
        st->st_mode = S_IFREG | cur_node->mode;
    }
    if (cur_node->flags == 0) {
        st->st_mode = S_IFDIR | cur_node->mode;
    }
    if (cur_node) {
        st->st_size = cur_node->size;
    }
    else {
        st->st_size = 0;
    }
    return 0;
}

const char*
get_data(const char* path)
{
    dir_ent* dat = get_file_data(path);
    if (!dat) {
        return 0;
    }

    inode* node = (inode*)get_pointer(dat->node_off);
    size_t size = node->size;
    int num_blocks = node->num_blocks;
    char* buf = malloc(num_blocks * BLOCK_SIZE);
    int* temp_pointer = 0;

    // copy all bytes from all blocks.
    for(int i = 0; i < num_blocks; i++) {
        temp_pointer = ((int*)get_pointer(node->blocks_off)) + i;
        char* temp = (char*)get_pointer(*(temp_pointer));
        for(int j = 0; j < BLOCK_SIZE; j++) {
            *(buf + j + (i * BLOCK_SIZE)) = *(temp + j);
        }
    }
    return buf;
}

int
rename_file(const char *from_path, const char *to_path)
{
    dir_ent* from_data = get_file_data(from_path);
    char* from_name = (char*)get_pointer(from_data->name_off);

    slist* path_name = s_split(to_path, '/');
    strcpy(from_name, path_name->next->data);

    return 0;
}

int
get_dirent_index(const char *path)
{
    directory* root = get_root_directory();

    for (int i = 0; i < root->inum; i++) {
        dir_ent* cur_ent = (dir_ent*)get_pointer(root->ents_off + (i * DIRENT_SIZE));
        const char* cur_name = (char*)get_pointer(cur_ent->name_off);
        if (streq(cur_name, path)) {
            return i;
        }
    }
    return 0;
}

int
unlink_file(const char *path)
{
    dir_ent* path_ent = get_file_data(path);
    if (path_ent == 0) {
        return -1;
    }
    inode* node = (inode*)get_pointer(path_ent->node_off);
    node->links_count -= 1;

    if (node->links_count <= 0) {
        // remove node and dir_ent.
        int index = path_ent->node_off / INODE_SIZE;
        memset((void*)node, 0, INODE_SIZE);
        set_bit((char*)get_pointer(s_block->inode_bitmap_off), s_block->inode_bitmap_size, 0, index);
    }

    // remove dir_ent
    // int ent_index = get_dirent_index(path);
    memset((void*)path_ent, 0, DIRENT_SIZE);

    directory* root = get_root_directory();
    root->inum -= 1;

    if (root->inum > 0) {
        dir_ent* last_ent = ((dir_ent*)get_pointer(root->ents_off)) + root->inum;
        if (last_ent > path_ent) {
            memcpy((void*)path_ent, (void*)last_ent, DIRENT_SIZE);
            memset((void*)last_ent, 0, DIRENT_SIZE);
        }
    }
    return 0;
}

int
make_dir(const char *path, mode_t mode) {
    int index = get_bit_index((char*)get_pointer(s_block->inode_bitmap_off), s_block->inode_bitmap_size);
    set_bit((char*)get_pointer(s_block->inode_bitmap_off), s_block->inode_bitmap_size, 1, index);

    inode* node = (inode*)(get_pointer(s_block->inodes_off) + (index * INODE_SIZE));
    node->mode = mode;
    node->uid = getuid();
    node->size = 0;
    node->atime = time(NULL);
    node->ctime = time(NULL);
    node->mtime = time(NULL);
    node->gid = getgid();
    node->links_count = 1;
    node->num_blocks = 0;
    node->flags = 0;
    node->blocks_off = sizeof(inode) + (s_block->inodes_off + (index * INODE_SIZE));

    int* temp_pointer = (int*)get_pointer(((inode*)get_pointer(s_block->root_node_off))->blocks_off);

    directory* root = (directory*)get_pointer(*(temp_pointer));
    dir_ent* new_ent = (dir_ent*)(get_pointer(root->ents_off + (root->inum * DIRENT_SIZE)));
    new_ent->node_off = s_block->inodes_off + (index * INODE_SIZE);
    new_ent->name_off = root->ents_off + ((root->inum + 2) * DIRENT_SIZE);
    slist* path_name = s_split(path, '/');
    char* name = (char*)get_pointer(new_ent->name_off);
    strcpy(name, path_name->next->data);

    root->inum += 1;

    return 0;
}
