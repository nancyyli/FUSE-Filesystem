#ifndef INODE_H
#define INODE_H


#include <unistd.h>
#include <sys/types.h>


typedef struct inode {
	mode_t mode; //read/write/execute
	uid_t uid; // user ID of the file owner
	off_t size; // size of the file in bytes
	time_t atime; // last access time
	time_t ctime; // creation time
	time_t mtime; // last modification time
	time_t dtime; // deletion time
	gid_t gid; // group ID of the file
	nlink_t links_count; // number of hard links pointing to this file
	int block; // number of blocks allocated to this file
	int flags; // file or directory?
	void* blocks[15];
} inode;



#endif
