#ifndef DIRECTORY_H
#define DIRECTORY_H

typedef struct dir_ent {
    int node_off; // node offset of directory entry
    int name_off; // name of directory entry. 48 bytes total.
    // const char* name;
} dir_ent;

typedef struct directory {
    int     inum;  // number of entries in directory
    int  node_off; // the node offset of this directory
    int ents_off;  // a list of entries
} directory;

#endif
