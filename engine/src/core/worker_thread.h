#pragma once

#include "containers/queue.h"
#include "core/mutex.h"
#include "core/ksemaphore.h"
#include "core/thread.h"
#include "defines.h"

typedef struct worker_thread {
    kthread thread;
    queue work_queue;
    kmutex queue_mutex;
} worker_thread;

API b8 worker_thread_create(worker_thread* out_thread);

API void worker_thread_destroy(worker_thread* thread);

API b8 worker_thread_add(worker_thread* thread, pfn_thread_start work_fn, void* params);

API b8 worker_thread_start(worker_thread* thread);

API b8 worker_thread_wait(worker_thread* thread);