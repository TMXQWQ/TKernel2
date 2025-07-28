#ifndef KERNEL_H
#define KERNEL_H
#include "limine.h"
#include "os_terminal.h"
extern struct limine_framebuffer *framebuffer;
void TK_logo();
#define sc(name) void _##name();
#include "syscmd.h"
void shell();
#endif