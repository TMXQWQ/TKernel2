#include "ttys.h"
#include "alloc.h"
#include "dpi.h"
#include "hal_io.h"

driver_handle ttys_handle = _BUILD_DRIVER_HANDLE(ttys);

driver *ttys_open(driver *d, uint64_t index) {
  init_serial();
  d->name = "TTY Serial";
  d->present = 1;
  d->major = "ttys";
  d->index = 1;
  d->handle = (driver_handle *)malloc(sizeof(driver_handle));
  return d;
}
driver *ttys_close(driver *d, uint64_t index) { return d = NULL; }
driver *ttys_read(driver *d, const char *minor, void *buf, uint64_t size,
                  uint64_t offset) {
  if (offset == 0 && size != 0) {
    for (int i = 0; i < size; i++) {
      char tmp = gets_serial();
      if (tmp == '\n') {
        ((char *)buf)[i] = '\0';
        break;
      }
      ((char *)buf)[i] = tmp;
    }
    return d;
  }
  return NULL;
}
driver *ttys_write(driver *d, const char *minor, const void *buf, uint64_t size,
                   uint64_t offset) {
  if (offset == 0 && size != 0) {
    for (int i = 0; i < size; i++) {
      putc_serial(((char *)buf)[i]);
    }
    return d;
  }
  return NULL;
}
driver *ttys_map(driver *d, const char *minor) { return NULL; }
driver *ttys_ioctl(driver *d, const char *minor, const char *cmd, ...) {
  return NULL; // TODO
}
char **ttys_get_minor(driver *d) { return NULL; } // No minor
int64_t *ttys_get_index(uint64_t size) {
  int64_t *ret = (int64_t *)malloc(sizeof(int64_t));
  *ret = 0;
  return ret; // Only one
}
