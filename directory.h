#ifndef DIRECTORY_H
#define DIRECTORY_H

typedef struct dir_ent {
    int node_off;      // node offset of directory entry
    const char*  name; // name of directory entry
} dir_ent;

typedef struct directory {
    int     inum;  // number of entries in directory
    int  node_off; // the node offset of this directory
    dir_ent* ents;  // a list of entries
} directory;

#endif
