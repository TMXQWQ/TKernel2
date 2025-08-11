#include "dpi.h"
#include "stddef.h"
#include "stdint.h"

driver static driver_list[MAX_DRIVER_NUM];

driver *reg_driver(driver *d) {
  if (d == NULL) {
    return NULL;
  }
  if (d->present != 1) {
    if (driver_list[d->index].name == d->name) {
      d->present = 1;
      return d;
    }
    for (register uint_fast64_t i = 0; i < MAX_DRIVER_NUM; i++) {
      if (driver_list[i].name == d->name) {
        d->present = 1;
        return d;
      }
    }
  }

  return d;
}
driver *get_driver(const char *str); //获取驱动指针