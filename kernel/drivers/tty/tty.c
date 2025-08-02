#include "tty.h"
#include "alloc.h"
#include "dpi.h"
#include "klibc.h"
#include "ttys.h"
#include "vargs.h"

list_t *tty_list = NULL;
driver *tty_open(driver *d, uint64_t index) {
  d->name = "TTY";
  d->major = "tty";
  d->present = 1;
  d->type = CHARACTER;
  d->index = index;
  d->private = (void *)malloc(sizeof(tty_private));
  if (tty_list == NULL) {
    tty_list = (list_t *)malloc(sizeof(list_t));
    tty_list->data = (void *)d;
    tty_list->next = tty_list->pre = tty_list;
    return d;
  }
  tty_list->pre->next = (void *)malloc(sizeof(list_t));
  tty_list->pre->next->pre = tty_list->pre;
  tty_list->pre->next->next = tty_list;
  tty_list->pre->next->data = (void *)d;
  tty_list->pre = tty_list->pre->next;
  return d;
}
driver *tty_close(driver *d, uint64_t index) { return d = NULL; }
driver *tty_read(driver *d, const char *minor, void *buf, uint64_t size,
                 uint64_t offset);
driver *tty_write(driver *d, const char *minor, const void *buf, uint64_t size,
                  uint64_t offset);
driver *tty_map(driver *d, const char *minor) { return NULL; }
driver *tty_ioctl(driver *d, const char *minor, const char *cmd, ...) {
  if (d == NULL) {
    return d;
  }

  tty_private *p = (tty_private *)d->private;
  if (!strcmp(cmd, "set")) {
    va_list args;
    va_start(args, cmd);
    driver *type = va_arg(args, driver *);
    if (!(p->d = type))
      return NULL;
    va_end(args);
  }

  return d;
}
char **tty_get_minor(driver *d) {
  if (d == NULL) {
    return NULL;
  }

  return d->handle->get_minor;
}
int64_t *tty_get_index(uint64_t size) {
  uint64_t *ret = (uint64_t *)malloc(size * sizeof(uint64_t));
  list_t *p = tty_list;
  for (uint64_t i = 0; i < size && p->next != tty_list; i++, p = p->next) {
    driver *d = (driver *)p->data;
    ret[i] = d->index;
  }
  return ret;
}
