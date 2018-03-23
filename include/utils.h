#ifndef _UTILS_H_
#define _UTILS_H_

#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>

#define TRUE    (1)
#define FALSE   (0)

#define ___constant_swab64(x) ((uint64_t)(             \
    (((uint64_t)(x) & (uint64_t)0x00000000000000ffULL) << 56) |   \
    (((uint64_t)(x) & (uint64_t)0x000000000000ff00ULL) << 40) |   \
    (((uint64_t)(x) & (uint64_t)0x0000000000ff0000ULL) << 24) |   \
    (((uint64_t)(x) & (uint64_t)0x00000000ff000000ULL) <<  8) |   \
    (((uint64_t)(x) & (uint64_t)0x000000ff00000000ULL) >>  8) |   \
    (((uint64_t)(x) & (uint64_t)0x0000ff0000000000ULL) >> 24) |   \
    (((uint64_t)(x) & (uint64_t)0x00ff000000000000ULL) >> 40) |   \
    (((uint64_t)(x) & (uint64_t)0xff00000000000000ULL) >> 56)))

/******************************************************************************/
// Thread Util Functions
/******************************************************************************/

static inline int create_thread(void *(*start_routine)(void *), void *arg)
{
    pthread_t tid;
    if (0 == pthread_create(&tid, NULL, start_routine, arg))
    {
        pthread_detach(tid);
        return 0;
    }

    return -1;
}

/******************************************************************************/
// Time Util Functions
/******************************************************************************/

static inline uint64_t current_time_millis()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

#endif // _UTILS_H_