#pragma once
#include "vfs.h"

typedef struct _ramfs_inode_block
{
    void* addr; //在内存中的地址
    size_t size;    //大小,以4KB为单位,最小4KB
    // struct _ramfs_inode_block* next;
} ramfs_block;

// ramfs私有inode
typedef struct _ramfs_inode
{
    ramfs_block *blocks;
    // size_t count;   //辅助:blocks的层数
} ramfs_inode;



int ramfs_mnt(vfs_inode_t* inode);
int ramfs_umnt();

int ramfs_open(vfs_inode_t* inode);
ssize_t ramfs_read(vfs_inode_t* inode, void* buf, size_t size, off_t offset);
ssize_t ramfs_write(vfs_inode_t* inode, const void* buf, size_t size, off_t offset);
void* ramfs_load(vfs_inode_t* inode);
int ramfs_sync(vfs_inode_t* inode);
int ramfs_close(vfs_inode_t* inode);
int ramfs_creat(vfs_inode_t* inode);

extern vfs_file_operations ramfs_ops;

