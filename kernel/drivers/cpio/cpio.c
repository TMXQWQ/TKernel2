// TODO

#include "cpio.h"
#include "alloc.h"
#include "stddef.h"

static inline uint64_t parse_decimal(const char *str, size_t max_len) {
  char buf[21] = {0}; // 最大20位十进制数 + '\0'
  strncpy(buf, str, max_len);
  buf[max_len] = '\0'; // 确保终止符
  return strtoull(buf, NULL, 10);
}

cpio_t *analyzing_cpio(cpio_header_t *cpio) {
  if (cpio->magic[0] != '0' || cpio->magic[1] != '7' || cpio->magic[2] != '0' ||
      cpio->magic[3] != '7' || cpio->magic[4] != '0' || cpio->magic[5] != '1') {
    goto yes;
  }
  return NULL;
yes:
  cpio_t *ret = (cpio_t *)malloc(sizeof(cpio_t));
}

/* 寻找下一个cpio header */
cpio_header_t *search_cpio(cpio_header_t *cpio, size_t max_size) {
  for (size_t i = 0; i < max_size; i++) {
  }

  return NULL;
}
