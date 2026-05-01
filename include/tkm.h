#ifndef TKM_H
#define TKM_H
#include "elf.h"
#include "kernel.h"
#include <stdint.h>

typedef struct {
        char *name;
        uintptr_t (*init)(); //初始化模块
} module_info;

typedef module_info *(*mod_enter)(kernel_info *);

int init_mod();

module_info *load_mod(Elf64_Ehdr *);

#endif
