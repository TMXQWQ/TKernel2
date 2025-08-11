#pragma once
#include "stdint.h"

typedef struct __attribute__((packed)) {
    char magic[6];       // 魔数 "070701" (newc格式标识)
    char ino[8];         // inode号 (ASCII十进制)
    char mode[8];        // 文件权限 (ASCII八进制)
    char uid[8];         // 用户ID (ASCII十进制)
    char gid[8];         // 组ID (ASCII十进制)
    char nlink[8];       // 硬链接数 (ASCII十进制)
    char mtime[8];       // 修改时间戳 (ASCII十进制)
    char filesize[8];    // 文件大小 (ASCII十进制)
    char devmajor[8];    // 主设备号 (ASCII十进制)
    char devminor[8];    // 次设备号 (ASCII十进制)
    char rdevmajor[8];   // 设备文件主号 (ASCII十进制)
    char rdevminor[8];   // 设备文件次号 (ASCII十进制)
    char namesize[8];    // 文件名长度 (ASCII十进制)
    char checksum[8];    // 校验和 (ASCII十进制)
    char name[];         // 文件名 (以'\0'结尾)
    // 文件内容紧随其后
} cpio_header_t;

typedef struct  {
    uint64_t ino;         // inode号 
    uint64_t mode;        // 文件权限 
    uint64_t uid;         // 用户ID 
    uint64_t gid;         // 组ID 
    uint64_t nlink;       // 硬链接数 
    uint64_t mtime;       // 修改时间戳 
    uint64_t filesize;    // 文件大小 
    uint64_t devmajor;    // 主设备号 
    uint64_t devminor;    // 次设备号 
    uint64_t rdevmajor;   // 设备文件主号 
    uint64_t rdevminor;   // 设备文件次号 
    uint64_t namesize;    // 文件名长度 
    uint64_t checksum;    // 校验和 
    char* name;         // 文件名 (以'\0'结尾)
    void* file;         //文件内容(filesize=0时为null)
} cpio_t;


