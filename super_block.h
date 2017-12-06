#ifndef SUPER_BLOCK_H
#define SUPER_BLOCK_H


#include <unistd.h>
#include "inode.h"

typedef struct super_block {
    // inode stuff
    int inode_bitmap_size; // total number of chars used for the inode bitmap.
    int inode_bitmap_off;  // offset of the bitmap for inodes. (char*)
    int inode_num;         // total number of inodes in disk.
    int inodes_off;        // offset of inodes in disk. (inode*)

    // data block stuff
    int data_bitmap_size;  // total number of chars used for the data bitmap.
    int data_bitmap_off;   // offset of the bitmap for data blocks in disk. (char*)
    int data_num;          // total number of data blocks in disk.
    int data_blocks_off;   // offset of the data blocks in disk. (void*)

    // root node stuff
    int root_node_off;     // offset of root node in the inode block.
} super_block;

#endif
