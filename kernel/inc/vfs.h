/*---------- vfs.h ----------*/
#pragma once
#include "ctype.h"
#include <stdint.h>
#include "klibc.h"

typedef struct vfs_filesystem vfs_filesystem_t;

#define EPERM   1   // 操作不允许
#define ENOENT  2   // 文件不存在
#define EIO     5   // I/O错误
#define EBADF   9   // 错误的文件描述符
#define EACCES  13  // 权限拒绝
#define EINVAL  22  // 无效参数
#define ENOSYS  38  // 功能未实现
#define EETYPE 20  // 不是目录/不是文件, 错误类型

#define VFS_NONE 0    // 无效节点
#define VFS_FILE 1    // 常规文件
#define VFS_DIR  2    // 目录
#define VFS_LINK 3    // 符号链接

// 通用VFS结构
typedef struct vfs_inode {
    uint64_t id;         // inode 编号
    char* name;
    uint32_t i_size;        // 文件大小
    list_t    file_list;     //目录下的文件
    void*    i_private;     // 私有数据指针,当type=link时指向对应的vfs_inode_t
    vfs_filesystem_t*    i_mnt;         // mnt数据指针(文件所在文件系统的指针)
                                        // type=link时无效
    uint16_t type;  //0:NULL(空,不存在)
                    //1:file
                    //2:dir
                    //3:link
} vfs_inode_t;

extern vfs_inode_t root;

typedef struct {
    int (*mnt)(vfs_inode_t* inode);
    int (*umnt)();
    int (*open)(vfs_inode_t* inode);
    ssize_t (*read)(vfs_inode_t* inode, void* buf, size_t size, off_t offset);
    ssize_t (*write)(vfs_inode_t* inode, const void* buf, size_t size, off_t offset);
    // 加载一个文件到内存
    void* (*load)(vfs_inode_t* inode);
    int (*sync)(vfs_inode_t* inode);
    int (*close)(vfs_inode_t* inode);
    int (*creat)(vfs_inode_t* inode);
} vfs_file_operations;

typedef struct vfs_filesystem {
    const char* name;
    vfs_inode_t* id; //挂载的inode编号
    vfs_file_operations ops;
    void*       fs_private;
} vfs_filesystem_t;

// extern list_t* fs_list;

int vfs_init();

int vfs_mount(vfs_filesystem_t fs,vfs_inode_t* inode);
int vfs_unmount(vfs_filesystem_t fs);


/* 核心操作函数 */
// 打开文件/目录
vfs_inode_t* vfs_open(const char* path);

// 读取文件内容
ssize_t vfs_read(vfs_inode_t* inode, void* buf, size_t size, off_t offset);

// 写入文件内容
ssize_t vfs_write(vfs_inode_t* inode, const void* buf, size_t size, off_t offset);

// 关闭文件
int vfs_close(vfs_inode_t* inode);

// 同步文件数据到存储设备
int vfs_sync(vfs_inode_t* inode);

// 将整个文件加载到内存
void* vfs_load(vfs_inode_t* inode);

vfs_inode_t* vfs_creat(const char* path, int flag);

/* 辅助函数 */
// 检查inode是否为目录
static inline int vfs_is_dir(vfs_inode_t* inode) {
    return inode && inode->type == 2;
}

// 检查inode是否为常规文件
static inline int vfs_is_file(vfs_inode_t* inode) {
    return inode && inode->type == 1;
}

// 检查inode是否为符号链接
static inline int vfs_is_link(vfs_inode_t* inode) {
    return inode && inode->type == 3;
}

vfs_inode_t* vfs_search(const char* path);

char* vfs_basename(const char* path,int n) ;

list_t* vfs_printk_ls(vfs_inode_t* i);

vfs_inode_t* _vfs_search(const char* path);

int vfs_token_count(const char *path);
