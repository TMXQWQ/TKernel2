#ifndef RPC_H
#define RPC_H

#define rpc_call(ns, name, ...) (rpc_get(ns, name))(__VA_ARGS__);
#include <stdint.h>
typedef uintptr_t (*rpc_func)(uintptr_t, ...);

typedef struct {
        int (*rpc_register)(const char *ns, const char *name, rpc_func func);
        rpc_func (*rpc_get)(const char *ns, const char *name);
} rpc_ops;

int      rpc_init();
int      rpc_register(const char *ns, const char *name, rpc_func func);
rpc_func rpc_get(const char *ns, const char *name);

#endif
