#include "spin.h"
#include "stdint.h"

/* Lock a spinlock */
void spin_lock(spinlock_t *lock) {
  __asm__ volatile("pushfq; pop %0; cli" : "=r"(lock->rflags));
  while (1) {
    uint64_t desired = 1;
    __asm__ volatile("lock xchg %[desired], %[lock];"
                     : [lock] "+m"(lock->lock), [desired] "+r"(desired)
                     :
                     : "memory");
    if (!desired)
      break;
    __asm__ volatile("pause");
  }
}

/* Unlock a spinlock */
void spin_unlock(spinlock_t *lock) {
  lock->lock = 0;
  __asm__ volatile("push %0; popfq" ::"r"(lock->rflags));
}
