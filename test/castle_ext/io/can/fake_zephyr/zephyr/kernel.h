// Minimal host-side stand-in for <zephyr/kernel.h> (timeouts and counting semaphores). The fake
// semaphore never blocks: a take on an empty semaphore fails immediately with the error the real
// kernel reports for K_NO_WAIT (-EBUSY) or an expired timeout (-EAGAIN).
#ifndef FAKE_ZEPHYR_KERNEL_H
#define FAKE_ZEPHYR_KERNEL_H

#include <errno.h>
#include <stdint.h>

typedef struct
{
    int64_t ticks;
} k_timeout_t;

#define K_NO_WAIT (k_timeout_t{0})
#define K_FOREVER (k_timeout_t{-1})
#define K_MSEC(ms) (k_timeout_t{static_cast<int64_t>(ms)})

#define K_SEM_MAX_LIMIT 0xFFFFFFFFU

struct k_sem
{
    unsigned int count;
    unsigned int limit;
};

inline int k_sem_init(struct k_sem* sem, unsigned int initial_count, unsigned int limit)
{
    sem->count = initial_count;
    sem->limit = limit;
    return 0;
}

inline int k_sem_take(struct k_sem* sem, k_timeout_t timeout)
{
    if (sem->count > 0U)
    {
        --sem->count;
        return 0;
    }
    return (timeout.ticks == 0) ? -EBUSY : -EAGAIN;
}

inline void k_sem_give(struct k_sem* sem)
{
    if (sem->count < sem->limit)
    {
        ++sem->count;
    }
}

inline void k_sem_reset(struct k_sem* sem)
{
    sem->count = 0U;
}

#endif // FAKE_ZEPHYR_KERNEL_H
