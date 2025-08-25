/*
 *
 *      dpi.h
 *      Driver Program Interface
 *
 *      2025/1/9 By TMXQWQ
 *      Based on GPL-3.0 open source agreement
 *      Copyright © 2020 ViudiraTech, based on the GPLv3 agreement.
 *
 */
#pragma once
#include "stdint.h"
#define MAX_DRIVER_NUM (0x20)

#define BUILD_DRIVER_HANDLE(name) \
        driver *name##_open(driver *d, uint64_t index);  \
        driver *name##_close(driver *d, uint64_t index);  \
        driver *name##_read(driver *d, const char *minor, void *buf, uint64_t size, uint64_t offset);  \
        driver *name##_write(driver *d, const char *minor, const void *buf, uint64_t size, uint64_t offset);  \
        driver *name##_map(driver *d, const char *minor);  \
        driver *name##_ioctl(driver *d, const char *minor, const char *cmd, ...);  \
        char **name##_get_minor(driver *d);  \
        int64_t *name##_get_index(uint64_t size);

#define _BUILD_DRIVER_HANDLE(name) { \
        .open = name##_open,  \
        .close = name##_close,  \
        .read = name##_read,  \
        .write = name##_write,  \
        .map = name##_map,  \
        .ioctl = name##_ioctl,  \
        .get_minor = name##_get_minor,  \
        .get_index = name##_get_index      \
}

struct Driver;
typedef struct Driver driver;

typedef struct Driver_Handle //驱动处理程序
{
        //minor指次设备号且!=0,minor=0表示不使用次设备号(忽略)
        driver *(*open)(driver *d, uint64_t index);
        driver *(*close)(driver *d, uint64_t index);
        uint64_t (*read)(driver *d, const char *minor, void *buf, uint64_t size, uint64_t offset); //读块/字符
        uint64_t (*write)(driver *d, const char *minor, const void *buf, uint64_t size, uint64_t offset); //写块/字符
        uint64_t *(*map)(driver *d, const char *minor); //映射到内存(非固定位置,)(用于帧缓冲等块设备)
        uint64_t *(*ioctl)(driver *d, const char *minor, const char *cmd, ...);
        char **(*get_minor)(driver *d);
        int64_t *(*get_index)(uint64_t size); //获取所有设备
} driver_handle;

typedef struct Driver //驱动结构体
{
        char *name;     //驱动程序名
        char *major;    //主设备号,用以标识驱动用于的设备(可能用于/dev)
        uint64_t index; //编号,用于区分不同设备(比如同时有两个ide)
        enum {
            CHARACTER, //字符设备
            BLOCK      //块设备
        } type;
        uint8_t present;       //是否有效
        driver_handle *handle; //处理程序
        void* private;  //私有数据(可选)
        uint64_t id;
} driver;

driver* reg_driver(driver* d);  //注册一个驱动
driver* get_driver(const char* str);    //获取驱动指针

