#include "namespace.h"
#include <string.h>

int ns_id     = 0;
ns  kernel_ns = {"Kernel", 0, &kernel_ns};

int ns_init()
{
    return 0;
}

int ns_register(ns *n)
{
    n->ns_id = ns_id++;
    ns *p    = kernel_ns.next;
    for (; p != &kernel_ns; p = p->next);
    p->next = n;
    n->next = &kernel_ns;
    return 0;
}

int ns_get(const char *name)
{
    ns *p = kernel_ns.next;
    for (; strcmp(p->name, name) || p==&kernel_ns; p = p->next);
    return p->ns_id;
}
