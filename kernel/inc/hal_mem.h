// #ifndef MEMORY_H
// #define MEMORY_H

// #include <limine.h>
// #include <stdint.h>
// #include <stddef.h>
// #include <stdbool.h>
// #include <string.h>

// // 页大小（4KB）
// #define PAGE_SIZE 4096

// // 内存映射类型
// #define LIMINE_MEMMAP_USABLE 1
// #define LIMINE_MEMMAP_RESERVED 2
// #define LIMINE_MEMMAP_ACPI_RECLAIMABLE 3
// #define LIMINE_MEMMAP_ACPI_NVS 4
// #define LIMINE_MEMMAP_BAD_MEMORY 5

// // 基本页管理常量
// #define PAGE_SIZE        4096
// #define PAGE_MASK        (~(PAGE_SIZE - 1ULL))
// #define PAGE_ALIGN(addr) ((addr + PAGE_SIZE - 1) & PAGE_MASK)

// // 页表项标志位
// #define PTE_PRESENT      (1ULL << 0)
// #define PTE_HUGE         (1ULL << 7)
// #define PTE_PRESENT    (1ULL << 0)  // 页存在
// #define PTE_WRITEABLE  (1ULL << 1)  // 可写
// #define PTE_USER       (1ULL << 2)  // 用户可访问
// #define PTE_NX         (1ULL << 63) // 禁止执行

// // 物理内存管理器
// typedef struct {
//     uint8_t *bitmap;           // 位图，用于管理物理页
//     size_t total_pages;        // 总物理页数
//     size_t last_allocated_page; // 最后分配的页，用于优化分配
// } PhysicalMemoryManager;

// // 虚拟内存管理器
// typedef struct {
//     uintptr_t *pml4_table;     // PML4 表（x86-64 的四级页表）
// } VirtualMemoryManager;

// // 函数声明
// void mem_init();
// void *page_alloc(size_t num);
// void page_free(void *addr, size_t num);
// void map_page(uintptr_t virtual_addr, uintptr_t physical_addr, uint64_t flags);
// void unmap_page(uintptr_t virtual_addr);

// #endif // MEMORY_H

/* memory.h */
#ifndef MEMORY_H
#define MEMORY_H

#include <limine.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGE_SIZE 4096
#define KERNEL_VIRT_BASE 0xffffffff80000000

// 内存映射类型
#define LIMINE_MEMMAP_USABLE 1
#define LIMINE_MEMMAP_RESERVED 2
#define LIMINE_MEMMAP_ACPI_RECLAIMABLE 3
#define LIMINE_MEMMAP_ACPI_NVS 4
#define LIMINE_MEMMAP_BAD_MEMORY 5

// 页表项标志位
#define PTE_PRESENT      (1ULL << 0)
#define PTE_HUGE         (1ULL << 7)
#define PTE_PRESENT    (1ULL << 0)  // 页存在
#define PTE_WRITEABLE  (1ULL << 1)  // 可写
#define PTE_USER       (1ULL << 2)  // 用户可访问
#define PTE_NX         (1ULL << 63) // 禁止执行

typedef enum {
    MEM_TYPE_FREE = 0,
    MEM_TYPE_RESERVED,
    MEM_TYPE_ACPI,
    MEM_TYPE_NVS
} MemoryType;

typedef struct {
    uint8_t* bitmap_phys;
    uint8_t* bitmap_virt;
    size_t total_pages;
    size_t last_scan;
} PhysicalMemoryManager;

typedef struct {
    uintptr_t* pml4;
} VirtualMemoryManager;

// 初始化内存管理系统
void mem_init();

// 物理页管理
void* page_alloc(size_t pages);
void page_free(void* addr, size_t pages);

// 虚拟内存管理
void map_page(uintptr_t virt, uintptr_t phys, uint64_t flags);
void unmap_page(uintptr_t virt);

#endif // MEMORY_H