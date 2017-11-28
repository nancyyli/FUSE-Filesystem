#ifndef SUPER_BLOCK_H
#define SUPER_BLOCK_H


#include <unistd.h>

typedef struct super_block {
      int inode_bitmap_size;
      void* inode_bitmap_addr;
      int data_bitmap_size;
      void* data_bitmap_addr;
      int inode_num;
      void* inode_addr;
      int data_num;
      void* data_addr;
      // index of root inode?
}super_block;

#endif
