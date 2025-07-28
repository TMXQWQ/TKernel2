#include "alloc.h"
#include "klibc.h"

void *kmalloc(unsigned long size) { return malloc(size); }

void kfree(void *ptr) {
  free(ptr);
  return;
}

void *calloc(size_t n, size_t size) {
  if (__builtin_mul_overflow(n, size, &size))
    return NULL;
  void *ptr = malloc(size);
  if (ptr == NULL)
    return NULL;
  memset(ptr, 0, size);
  return ptr;
}

void *realloc(void *ptr, size_t newsize) {
  void *tmp = malloc(newsize);
  memcpy(tmp, ptr, newsize);
  return tmp;
}