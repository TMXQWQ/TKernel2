// TODO(肝不动力qwq)

#include "rpc.h"
#include "printk.h"
#include <stdint.h>

int rpc_init()
{
    rpc_register("kernel", "printk", (rpc_func)(uintptr_t)printk);
    return 0;
}

int rpc_register(const char *ns, const char *name, rpc_func func)
{
}
rpc_func rpc_get(const char *ns, const char *name)
{
}
