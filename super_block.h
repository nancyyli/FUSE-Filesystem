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
    int data_blocks_off;     // offset of the data blocks in disk. (void*)

    // root node stuff
    int root_node_off;     // offset of root node in the inode block.
} super_block;

#endif

/*
    path names:
        the data of an inode, if it is a directory, will contain the name
        of the current directory, and the children. each children will either
        be another directory of a file.

        /user/jlopez/hello.txt

        look in the root inode for user/:    user/     -> inode A
        look in inode A for jlopez/:         jlopez/   -> inode B
        look in inode B for hello.txt:       hello.txt -> "hello"

        we should use the provided slist, because it splits up a directory string
        into its parts.

        maybe also use the provided directory structure.

    bitmap:
        implement by using an array of char. access each bit using bit operations.
        if necessary, we could just copy an implementation from the internet
        and give attribution.

    inderect blocks:
        an inderect block points to a data block that has a bunch of pointers to
        other data blocks.

    TODO: determine the number of inodes to use.
*/
