#ifndef TKM_H
#define TKM_H
#include "kernel.h"
#include <stdint.h>

typedef struct {
        char *const name;
        uint8_t     nice;                 // 优先级，优先级低的先加载
        uintptr_t (*init)(kernel_info *); //初始化模块
} module_info;

typedef module_info* (*mod_enter)();
#endif
