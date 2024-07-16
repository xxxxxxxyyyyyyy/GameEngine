#ifndef _K_SEMAPHORE_H_
#define _K_SEMAPHORE_H_

#include "defines.h"

typedef struct ksemaphore {
    void *internal_data;
} ksemaphore;

API b8 ksemaphore_create(ksemaphore *out_semaphore, u32 max_count, u32 start_count);

API void ksemaphore_destroy(ksemaphore *semaphore);

API b8 ksemaphore_signal(ksemaphore *semaphore);

/**
 * Decreases the semaphore count by 1. If the count reaches 0, the
 * semaphore is considered unsignaled and this call blocks until the
 * semaphore is signaled by ksemaphore_signal.
 */
API b8 ksemaphore_wait(ksemaphore *semaphore, u64 timeout_ms);

#endif