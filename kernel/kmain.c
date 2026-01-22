#include "cpio.h"
#include "elf_parse.h"
#include "kernel.h"
#include "limine_module.h"
#include "printk.h"
#include "serial.h"
#include "tkm.h"
#include <stddef.h>
#include <stdint.h>

typeof(bootloader) bootloader;

kernel_info kinfo = {
    .kernel_name = KERNEL_NAME,
    .version     = KERNEL_VERSION,
};

void executable_entry(void)
{
    const char msg[] = "Logically you should use Limine to boot it instead of executing it directly, right?\n\n";
    for (;;);
    __asm__ volatile("mov $1, %%rax\n"
                     "mov $1, %%rdi\n"
                     "lea %[msg], %%rsi\n"
                     "mov %[len], %%rdx\n"
                     "syscall\n"
                     "mov $60, %%rax\n"
                     "mov $1, %%rdi\n"
                     "syscall\n"
                     :
                     : [msg] "m"(msg), [len] "r"(sizeof msg - 1)
                     : "rax", "rdi", "rsi", "rdx");
}

void kernel_entry(void)
{
    lmodule_t      *mod  = get_lmodule("initrd");
    newc_filesystem cpio = cpio_parse((newc_header *)mod->data);
    enter           test;
    for (size_t i = 0; i < cpio.size; i++) {
        char *tmp = cpio.file_list[i].name;
        for (int j = 0; tmp[j] != '\0'; j++)
            if (tmp[j] == '.' && tmp[j + 1] != '\0' && tmp[j + 2] != '\0' && tmp[j + 3] != '\0' && tmp[j + 1] == 't' && tmp[j + 2] == 'k'
                && tmp[j + 3] == 'm')
                test = elf_pie_enter_parse((Elf64_Ehdr *)cpio.file_list[i].data_ptr);
    }
    // 以下是实际上的内核主程序，以上内容以后要归到limine_enter里
    init_serial();
    printk("Boot By : %s\n", bootloader == Limine ? "Limine" : "Unkown");
    module_info* a = ((mod_enter)(uintptr_t)test)();
    printk("test returned:%d\n", a->init(&kinfo));
    for (;;);
}
