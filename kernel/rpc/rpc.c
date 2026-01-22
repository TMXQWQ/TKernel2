// TODO(肝不动力qwq)
// md我tm写的什么神经代码，我是煞笔草

#include "rpc.h"
#include "namespace.h"
#include "printk.h"
#include <stdint.h>

rpc_func rpc_list[MAX_RPC_NUM][MAX_RPC_NUM];

int rpc_init()
{
    rpc_register("kernel", "printk", (rpc_func)(uintptr_t)printk);
    return 0;
}

int rpc_register(const char *ns, const char *name, rpc_func func)
{
    int ns_id = ns_get(ns);
    rpc_list[ns_id];
}
rpc_func rpc_get(const char *ns, const char *name)
{
}
