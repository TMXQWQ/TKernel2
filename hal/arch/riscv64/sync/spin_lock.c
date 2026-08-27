/*
 *
 *      spin_lock.c
 *      Spin lock (RISC-V)
 *
 *      Ported from the x86_64 implementation.
 *      Uses the RISC-V atomic instruction amoswap.d with acquire/release
 *      ordering, and saves/restores the SIE bit in sstatus so that the
 *      critical section runs with interrupts disabled, matching the
 *      semantics of the x86_64 version.
 *
 */

#include "spin_lock.h"

/* Lock a spinlock */
void spin_lock(spinlock_t *lock)
{
    uint64_t sstatus;
    uint64_t one  = 1;
    uint64_t old;
    uint64_t addr = (uint64_t)&lock->lock;

    /* Save interrupt state and disable interrupts (clear SIE) */
    __asm__ volatile("csrr %0, sstatus" : "=r"(sstatus));
    lock->rflags = sstatus;
    __asm__ volatile("csrci sstatus, 2");

    while (1) {
        /* Atomically swap the lock word with 1; old==0 means we acquired it */
        __asm__ volatile("amoswap.d.aq %0, %2, (%1)"
                         : "=&r"(old)
                         : "r"(addr), "r"(one)
                         : "memory");
        if (!old) break;
    }
}

/* Unlock a spinlock */
void spin_unlock(spinlock_t *lock)
{
    uint64_t zero = 0;
    uint64_t addr = (uint64_t)&lock->lock;

    /* Atomically release the lock with release ordering */
    __asm__ volatile("amoswap.d.rl x0, %1, (%0)"
                     : : "r"(addr), "r"(zero)
                     : "memory");

    /* Restore the saved interrupt state */
    __asm__ volatile("csrw sstatus, %0" :: "r"(lock->rflags));
}
