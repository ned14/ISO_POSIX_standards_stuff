/* c1x_compat.h
Declares and defines stuff from C11
(C) 2011-2012 Niall Douglas http://www.nedproductions.biz/


Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#ifndef C11_COMPAT_H
#define C11_COMPAT_H

#include <stdlib.h>
#include <errno.h>
#ifndef ETIMEDOUT
#define ETIMEDOUT       138
#endif
#if __STDC_VERSION__ > 200000L
#include <stdatomic.h>
#include <threads.h>

#else // Need to fake C11 support

#ifdef _MSC_VER
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <intrin.h>
#include <process.h>
#endif
#if defined(__GNUC__) || defined(__clang__)
#ifdef _WIN32
#include <windows.h>
#endif
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#ifdef __clang__
#include "c11_atomics_clang/atomic"
#else
#include <atomic>
#endif
// Evilly patch in the C++11 atomics as if they were C11
#define memory_order_relaxed std::memory_order::memory_order_relaxed
#define memory_order_consume std::memory_order::memory_order_consume
#define memory_order_acquire std::memory_order::memory_order_acquire
#define memory_order_release std::memory_order::memory_order_release
#define memory_order_acq_rel std::memory_order::memory_order_acq_rel
#define memory_order_seq_cst std::memory_order::memory_order_seq_cst

#define atomic_uint std::atomic<unsigned int>
#define atomic_init std::atomic_init
#define atomic_thread_fence std::atomic_thread_fence
#define atomic_store_explicit std::atomic_store_explicit
#define atomic_load_explicit std::atomic_load_explicit
#define atomic_exchange_explicit std::atomic_exchange_explicit
#define atomic_compare_exchange_weak_explicit std::atomic_compare_exchange_weak_explicit
#define atomic_compare_exchange_strong_explicit std::atomic_compare_exchange_strong_explicit
#define atomic_fetch_add_explicit std::atomic_fetch_add_explicit
#endif

/* We need inline */
#if !defined(__cplusplus) && !defined(inline) && (defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__))
#define inline __inline
#endif

#if __STDC_VERSION__ < 199901L		/* not C99 or better */
#if !defined(RESTRICT) && (defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__))
#define RESTRICT __restrict
#endif
#endif

#ifndef RESTRICT
#define RESTRICT
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* We need bool */
#if defined(_MSC_VER) || defined(__clang__)
#ifdef __cplusplus
typedef bool _Bool;
#else
typedef unsigned char _Bool;
#endif
#endif


#ifndef _GLIBCXX_ATOMIC
/****************** Declare and define just those bits of stdatomic.h we need ***********************/
typedef unsigned int atomic_uint;

typedef enum memory_order
{
  memory_order_relaxed,
  memory_order_consume,
  memory_order_acquire,
  memory_order_release,
  memory_order_acq_rel,
  memory_order_seq_cst
} memory_order;

inline void atomic_init(volatile atomic_uint *o, unsigned int v)
{
  /* Both MSVC and GCC do the right thing when it's marked volatile */
  *o=v;
}

#ifdef _MSC_VER
inline void atomic_thread_fence(memory_order order)
{ /*
acquire = prevents memory operations after the acquire moving before but
permits previous operations to move downwards

release = prevents memory operations before the release moving after but
permits subsequent operations to move upwards
*/
  switch(order)
  {
  case memory_order_acquire:
  case memory_order_release:
  case memory_order_acq_rel:
  case memory_order_seq_cst:
    MemoryBarrier();
    return;
  default:
    return;
  }
}
inline void atomic_store_explicit(volatile atomic_uint *o, unsigned int v, memory_order order)
{
  switch(order)
  {
  case memory_order_relaxed:
    *(atomic_uint *) o=v;
    return;
  case memory_order_acquire:
  case memory_order_release:
  case memory_order_acq_rel:
  /* Both MSVC and GCC do the right thing when it's marked volatile and use acquire/release automatically */
    *o=v;
    return;
  case memory_order_seq_cst:
  default:
    InterlockedExchange((volatile long *) o, (long) v);
    return;
  }
}
inline unsigned int atomic_load_explicit(volatile atomic_uint *o, memory_order order)
{
  switch(order)
  {
  case memory_order_relaxed:
    return *(atomic_uint *) o;
  case memory_order_acquire:
  case memory_order_release:
  case memory_order_acq_rel:
  /* Both MSVC and GCC do the right thing when it's marked volatile and use acquire/release automatically */
    return *o;
  case memory_order_seq_cst:
  default:
    {
    atomic_uint ret;
    MemoryBarrier();
    ret=*o;
    MemoryBarrier();
    return ret;
    }
  }
}
inline unsigned int atomic_exchange_explicit(volatile atomic_uint *o, unsigned int v, memory_order order)
{
  switch(order)
  {
#ifdef InterlockedExchangeAcquire
  case memory_order_acquire:
    return (unsigned int) InterlockedExchangeAcquire((volatile long *) o, v);
  case memory_order_release:
    return (unsigned int) InterlockedExchangeRelease((volatile long *) o, v);
#endif
  default:
    return (unsigned int) InterlockedExchange((volatile long *) o, (long) v);
  }
}
inline unsigned int atomic_compare_exchange_weak_explicit(volatile atomic_uint *o, unsigned int *expected, unsigned int v, memory_order success, memory_order failure)
{
  unsigned int former, e=*expected;
  switch(success)
  {
#ifdef InterlockedExchangeAcquire
  case memory_order_acquire:
    former=(unsigned int) InterlockedCompareExchangeAcquire((volatile long *) o, (long) v, (long) e);
    break;
  case memory_order_release:
    former=(unsigned int) InterlockedCompareExchangeRelease((volatile long *) o, (long) v, (long) e);
    break;
#endif
  default:
    former=(unsigned int) InterlockedCompareExchange((volatile long *) o, (long) v, (long) e);
    break;
  }
  if(former==e) return 1;
  atomic_store_explicit(expected, former, failure);
  return 0;
}
inline unsigned int atomic_compare_exchange_strong_explicit(volatile atomic_uint *o, unsigned int *expected, unsigned int v, memory_order success, memory_order failure)
{
  /* No difference to weak when using Interlocked* functions */
  return atomic_compare_exchange_weak_explicit(o, expected, v, success, failure);
}
inline unsigned int atomic_fetch_add_explicit(volatile atomic_uint *o, unsigned int v, memory_order order)
{
  switch(order)
  {
#ifdef InterlockedExchangeAddAcquire
  case memory_order_acquire:
    return (unsigned int) InterlockedExchangeAddAcquire((volatile long *) o, v);
  case memory_order_release:
    return (unsigned int) InterlockedExchangeAddRelease((volatile long *) o, v);
#endif
  default:
    return (unsigned int) InterlockedExchangeAdd((volatile long *) o, (long) v);
  }
}
#endif
#ifdef __GNUC__
#error Awaiting implementation
#endif
#endif // _GLIBCXX_ATOMIC



/****************** Declare and define just those bits of threads.h we need ***********************/
enum mtx_types
{
  mtx_plain=0,
  mtx_recursive=1,
  mtx_timed=2
};
enum thrd_results
{
  thrd_success=0,
  thrd_timeout=ETIMEDOUT,
  thrd_busy=EBUSY,
  thrd_error=EINVAL,
  thrd_nomem=ENOMEM
};

#ifdef _MSC_VER
struct timespec
{
  time_t tv_sec;
  long tv_nsec;
};

typedef CONDITION_VARIABLE cnd_t;
typedef SRWLOCK mtx_t;
#else
typedef pthread_cond_t cnd_t;
typedef pthread_mutex_t mtx_t;
#endif

#define TIME_UTC 1
inline int timespec_get(struct timespec *ts, int base)
{
#ifdef _MSC_VER
  static LARGE_INTEGER ticksPerSec;
  static double scalefactor;
  LARGE_INTEGER val;
  if(!scalefactor)
  {
	if(QueryPerformanceFrequency(&ticksPerSec))
		scalefactor=ticksPerSec.QuadPart/1000000000.0;
	else
		scalefactor=1;
  }
  if(!QueryPerformanceCounter(&val))
  {
    DWORD now=GetTickCount();
    ts->tv_sec=now/1000;
    ts->tv_nsec=(now%1000)*1000000;
    return base;
  }
  {
    unsigned long long now=(unsigned long long)(val.QuadPart/scalefactor);
    ts->tv_sec=now/1000000000;
    ts->tv_nsec=now%1000000000;
  }
  return base;
#else
#ifdef CLOCK_MONOTONIC
  clock_gettime(CLOCK_MONOTONIC, ts);
#else
  struct timeval tv;
  gettimeofday(&tv, 0);
  ts->tv_sec=tv.tv_sec;
  ts->tv_nsec=tv.tv_usec*1000;
#endif
  return base;
#endif
}
inline long long timespec_diff(const struct timespec *end, const struct timespec *start)
{
  long long ret=end->tv_sec-start->tv_sec;
  ret*=1000000000;
  ret+=end->tv_nsec-start->tv_nsec;
  return ret;
}

#ifdef _MSC_VER
inline int cnd_broadcast(cnd_t *cond) { WakeAllConditionVariable(cond); return thrd_success; }
inline void cnd_destroy(cnd_t *cond) { }
inline int cnd_init(cnd_t *cond) { InitializeConditionVariable(cond); return thrd_success; }
inline int cnd_signal(cnd_t *cond) { WakeConditionVariable(cond); return thrd_success; }
inline int cnd_timedwait(cnd_t *RESTRICT cond, mtx_t *RESTRICT mtx, const struct timespec *RESTRICT ts)
{
  struct timespec now;
  DWORD interval;
  timespec_get(&now, TIME_UTC);
  interval=(DWORD)(timespec_diff(ts, &now)/1000000);
  return SleepConditionVariableSRW(cond, mtx, interval, 0) ? thrd_success : thrd_timeout;
}
inline int cnd_wait(cnd_t *cond, mtx_t *mtx) { return SleepConditionVariableSRW(cond, mtx, INFINITE, 0) ? thrd_success : thrd_timeout; }

inline void mtx_destroy(mtx_t *mtx) { }
inline int mtx_init(mtx_t *mtx, int type) { InitializeSRWLock(mtx); return thrd_success; }
inline int mtx_lock(mtx_t *mtx) { AcquireSRWLockExclusive(mtx); return thrd_success; }
inline int mtx_timedlock(mtx_t *RESTRICT mtx, const struct timespec *RESTRICT ts);
inline int mtx_trylock(mtx_t *mtx) { return TryAcquireSRWLockExclusive(mtx) ? thrd_success : thrd_busy; }
inline int mtx_unlock(mtx_t *mtx) { ReleaseSRWLockExclusive(mtx); return thrd_success; }
#else
#define cnd_broadcast pthread_cond_broadcast
#define cnd_destroy pthread_cond_destroy
#define cnd_init(a) pthread_cond_init((a), NULL)
#define cnd_signal pthread_cond_signal
#define cnd_timedwait pthread_cond_timedwait
#define cnd_wait pthread_cond_wait

#define mtx_destroy pthread_mutex_destroy
#define mtx_init(m, a) pthread_mutex_init((m), NULL)
#define mtx_lock pthread_mutex_lock
#define mtx_timedlock fail
#define mtx_trylock fail
#define mtx_unlock pthread_mutex_unlock
#endif

#endif





/****************** Declare and define just those bits of threads.h we need ***********************/
#ifdef _MSC_VER
typedef int (*thrd_start_t)(void *);
typedef uintptr_t thrd_t;

inline int thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
  *thr=_beginthread((void (*)(void *)) func, 0, arg);
  return thrd_success;
}
inline int thrd_sleep(const struct timespec *duration, const struct timespec *remaining)
{
  Sleep((DWORD)(duration->tv_sec*1000+duration->tv_nsec/1000000));
  return thrd_success;
}
inline void thrd_yield(void)
{
  Sleep(0);
}
#else
typedef int (*thrd_start_t)(void *);
typedef pthread_t thrd_t;

inline int thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
  pthread_create(thr, NULL, (void *(*)(void *))func, arg);
  return thrd_success;
}
inline int thrd_sleep(const struct timespec *duration, struct timespec *remaining)
{
#ifdef _WIN32 // Mingw doesn't define a nanosleep in its libraries
  Sleep((DWORD)(duration->tv_sec*1000+duration->tv_nsec/1000000));
  return thrd_success;
#else
  return nanosleep(duration, remaining);
#endif
}
#define thrd_yield sched_yield
#endif

#ifdef __cplusplus
}
#endif

#endif

