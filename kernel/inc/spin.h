#pragma once

#include "stdint.h"

typedef struct {
        volatile uint64_t lock; // lock state
        uint64_t rflags;        // stored rflags
} spinlock_t;

/* Lock a spinlock */
void spin_lock(spinlock_t *lock);

/* Unlock a spinlock */
void spin_unlock(spinlock_t *lock);
