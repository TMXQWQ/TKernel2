#ifndef KPI_H
#define KPI_H

#include <stddef.h>
#include <stdint.h>
typedef struct module_info module_info;

uintptr_t    kpi_resolve_symbol(module_info *mod, const char *name);
void         kpi_register_module(module_info *mod);
module_info *find_module(const char *name); // 若需外部调用

typedef struct {
        // Memory Manage
        void *(*kmalloc)(size_t size);
        void (*kfree)(void* ptr);
        uint64_t (*alloc_frames)(size_t count);
        void (*free_frame)(uint64_t addr);
} kpi_t;

#endif