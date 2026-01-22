#ifndef NS_H
#define NS_H

typedef struct NameSpace {
        char             *name;
        int               ns_id;
        struct NameSpace *next; // list,虽然我觉得其实没必要在这写list()
} ns;

extern int ns_id;
extern ns  kernel_ns;

int ns_init();

int ns_register(ns *); // 返回id

int ns_get(const char *name); // 返回name对应的id,如果失败返回kernel_ns的name("Kernel")

#endif
