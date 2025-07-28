#include "ctypes.h"

char *strcpy(char *dest, const char *src) {
  // uint64_t rdi_val,rsi_val,rdx_val;
  // __asm__ __volatile__(
  //     "mov %%rdi, %0\n\t"
  //     "mov %%rsi, %1\n\t"
  //     "mov %%rdx, %2"
  //     : "=r"(rdi_val), "=r"(rsi_val), "=r"(rdx_val)
  // );
  // printks("RDI=%p RSI=%p RDX=%p\n", rdi_val, rsi_val, rdx_val);

  do {
    *dest++ = *src++;
  } while (*src != 0);
  *dest = 0;
  return dest;
}