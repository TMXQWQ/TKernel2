#ifndef KPI_H
#define KPI_H

#include "tkm.h"

uintptr_t kpi_resolve_symbol(module_info *mod, const char *name);
void kpi_register_module(module_info *mod);
module_info *find_module(const char *name);  // 若需外部调用

#endif