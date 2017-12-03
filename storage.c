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
const int INODE_SIZE =  sizeof(inode) + (sizeof(int*) * 15); // NOT TOO SURE ABOUT THIS

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
    TODO: delete later

void print_char_bin(char c)
{
    for (int i = 7; i >= 0; --i)
    {
        putchar( (c & (1 << i)) ? '1' : '0' );
    }
    putchar('\n');
}
*/

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

    //printf("pointer of s_block %p\n", (void*)s_block);
    s_block->inode_bitmap_size = 4; // TODO: pick better than just 4 chars
    s_block->inode_bitmap_off =  sizeof(super_block); // set offset of the bitmap #(void*)s_block + sizeof(super_block);
    //printf("inode_bitmap: %p\n", (void*) s_block->inode_bitmap);
    s_block->data_bitmap_size = 6;
    s_block->data_bitmap_off = s_block->inode_bitmap_off + s_block->inode_bitmap_size;
    s_block->inode_num = 10; // TODO: pick better number than just 10 inodes
    s_block->inodes_off = s_block->data_bitmap_off + s_block->data_bitmap_size;
    s_block->data_num = 45;
    s_block->data_blocks_off = s_block->inodes_off + (s_block->inode_num * INODE_SIZE);
    s_block->root_node_off = s_block->inodes_off;
    //printf("Made s_block\n");

    inode* r_node = (inode*)get_pointer(s_block->root_node_off);
    //printf("r_node: %p\n", (void*) r_node);
    r_node->mode = 0755;
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

    //printf("made r_node\n");
    directory* dir = (directory*)get_pointer(*(temp_pointer));
    dir->node_off = s_block->root_node_off;
    dir->inum = 1;
    dir->ents = (void*)dir + INODE_SIZE + sizeof(int);
    dir->ents->name = "/";
    dir->ents->node_off = s_block->root_node_off;

    printf("made directory\n");
    set_bit((char*)get_pointer(s_block->inode_bitmap_off), s_block->inode_bitmap_size, 1, 0);
    set_bit((char*)get_pointer(s_block->data_bitmap_off), s_block->data_bitmap_size, 1, 0);
    printf("Ending init\n");
    //    printf("size of inode: %d", sizeof(inode));
    //    printf("size of superblock: %d", sizeof(super_block));
    //implement index of root node for s_block
}

// TODO: if file name already exists
// the comment above can most likely be ignored.
// fuse checks if a file exists with one of our methods.
// so this is only called when we know the file does not exist.
int
make_file(const char *path, mode_t mode, dev_t rdev) {
    printf("making file: %s, %04o\n", path, mode);
    //print_char_bin(*(s_block->inode_bitmap));
    int index = get_bit_index((char*)get_pointer(s_block->inode_bitmap_off), s_block->inode_bitmap_size);

    printf("making file: %s with inode_bitmap index: %d\n", path, index);
    set_bit((char*)get_pointer(s_block->inode_bitmap_off), s_block->inode_bitmap_size, 1, index);
    /*
    int num_node = 0;
    inode* node = s_block->inodes;
    while(num_node < index) {
        printf("num_node: %d, index: %d\n", num_node, index);
        node->next = (void*) node + sizeof(inode);
        node = node->next;
        num_node += 1;
    }
    */
    //printf("s_block->inodes pointer: %p\n", (void*) s_block->inodes);
    //printf("node pointer: %p\n", (void*) node);
    inode* node = (inode*)get_pointer(s_block->inodes_off) + (index * INODE_SIZE);
    node->mode = mode;
    node->uid = getuid();
    node->size = 0;
    node->atime = time(NULL);
    node->ctime = time(NULL);
    node->mtime = time(NULL);
    node->gid = getgid();
    node->links_count = 0;
    node->num_blocks = 0;
    node->flags = 1; // TODO

    int* temp_pointer = (int*)get_pointer(((inode*)get_pointer(s_block->root_node_off))->blocks_off);

    directory* root = (directory*)get_pointer(*(temp_pointer));
    dir_ent* new_ent = root->ents + root->inum;
    new_ent->node_off = s_block->inodes_off + (index * INODE_SIZE);
    slist* path_name = s_split(path, '/');
    new_ent->name = path_name->next->data;
    root->inum += 1;
    printf("root->inum: %d\n", root->inum);

    return 0;
}

static dir_ent*
get_file_data(const char* path) {
    printf("going into get_file_data\n");
    int* temp_pointer = (int*)get_pointer(((inode*)get_pointer(s_block->root_node_off))->blocks_off);
    directory* root = (directory*)get_pointer(*(temp_pointer));
    if (streq(path, "/")) {
        printf("getting dirent for root\n");
        return root->ents;
    }
    slist* path_list = s_split(path,  '/');
    /*
    while (path_list != NULL) {
        printf("Printing data in path_list: %s\n", path_list->data);
        if (path_list->next != NULL) {
            path_list = path_list->next;
        }
        else {
            break;
        }
    }
    */
    path_list = path_list->next; // first is empty
    char* current = path_list->data;
    printf("current_path_list_data: %s\n", path_list->data);
    directory* temp = root;
    dir_ent* temp_ent = temp->ents;
    printf("going into while loop in get_file_data\n");
    while (path_list != NULL) {
        for(int i = 0; i < temp->inum; i++) {
            printf("going into for loop\n");
            printf("temp->inum: %d, i: %d\n", temp->inum, i);
            printf("path_list data: %s temp->ents->name: %s\n", current, temp_ent->name);
            if(streq(current, temp_ent->name)) {
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
                        temp_ent = temp->ents;
                        i = 0;
                        break;
                    }
                    else {
                        // maybe change for an error
                        printf("%s is an invalid path\n", path);
                        return 0;
                    }
                }
            }
            temp_ent = temp_ent + 1; // should only be plus 1 to move to the next dirent.
        }
        printf("path_list = path_list->next\n");
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
    if (!dat) {
        // TODO: print error? make new file? what should happen in this case?
        return -1;
    }

    inode* node = (inode*)get_pointer(dat->node_off);
    int* temp_pointer = 0;

    // add mem blocks to the node if necessary
    if((offset + size)/BLOCK_SIZE > node->num_blocks) {
        for(int i = 0; 0 < ((offset + size)/BLOCK_SIZE) - node->num_blocks; i++) {
            // get index from bitmap
            int index = get_bit_index((char*)get_pointer(s_block->data_bitmap_off), s_block->data_bitmap_size);
            set_bit((char*)get_pointer(s_block->data_bitmap_off), s_block->data_bitmap_size, 1, index);

            // set the block offeset in the node
            temp_pointer = ((int*)get_pointer(node->blocks_off))[node->num_blocks];
            *(temp_pointer) = (index * BLOCK_SIZE) + s_block->data_blocks_off;
            node->num_blocks += 1;
        }
    }

    // get the data from the block we are writting to
    temp_pointer = ((int*)get_pointer(node->blocks_off))[block_num];
    char* file_data = get_pointer(*(temp_pointer));

    // write the data
    for (int i = 0; i < size; i++) {
        // checks if we have reached the point to switch to a new block
        if ((offset + i) % BLOCK_SIZE == 0) {
            block_num += 1;
            off = 0;
            temp_pointer = ((int*)get_pointer(node->blocks_off))[block_num];
            file_data = get_pointer(*(temp_pointer));
        }

        // writting a byte at a time. might not work.
        *(file_data + off) = *(buf + i);
    }

    return 0;
}

int
file_exists(const char* path) {
    dir_ent* dat = get_file_data(path);
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
    char* buf = malloc(num_blocks * BLOCK_SIZE); // maybe change it so that it is the number of bytes in the file.
    int* temp_pointer = 0;

    // copy all bytes from all blocks.
    for(int i = 0; i < num_blocks; i++) {
        temp_pointer = ((int*)get_pointer(node->blocks_off))[i];
        char* temp = (char*)get_pointer(*(temp_pointer));
        for(int j = 0; i < BLOCK_SIZE; j++) {
            *(buf + j + (i * BLOCK_SIZE)) = *(temp + j);
        }
    }

    return buf;
}
