/* c1x_compat.h
Declares and defines stuff from C1X
(C) 2011 Niall Douglas http://www.nedproductions.biz/


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

#ifndef C1X_COMPAT_H
#define C1X_COMPAT_H

#include <stdlib.h>
#if __STDC_VERSION__ > 200000L || defined(__GNUC__)
#include <stdatomic.h>
#include <threads.h>
#else

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include <process.h>
#endif

/* We need inline */
#if !defined(__cplusplus) && !defined(inline) && (defined(_MSC_VER) || defined(__GNUC__))
#define inline __inline
#endif

#if __STDC_VERSION__ < 199901L		/* not C99 or better */
#if !defined(RESTRICT) && (defined(_MSC_VER) || defined(__GNUC__))
#define RESTRICT __restrict
#endif
#endif

#ifndef RESTRICT
#define RESTRICT
#endif


/****************** Declare and define just those bits of stdatomic.h we need ***********************/
typedef ptrdiff_t atomic_ptrdiff_t;

typedef enum memory_order
{
  memory_order_relaxed,
  memory_order_consume,
  memory_order_acquire,
  memory_order_release,
  memory_order_acq_rel,
  memory_order_seq_cst
} memory_order;

inline void atomic_init(volatile atomic_ptrdiff_t *o, size_t v)
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
inline void atomic_store_explicit(volatile atomic_ptrdiff_t *o, ptrdiff_t v, memory_order order)
{
  if(sizeof(ptrdiff_t)>4)
    switch(order)
    {
    default:
      InterlockedExchange64((volatile LONGLONG *) o, v);
      return;
    }
  else
    switch(order)
    {
    default:
      InterlockedExchange((volatile long *) o, (long) v);
      return;
    }
}
inline ptrdiff_t atomic_load_explicit(volatile atomic_ptrdiff_t *o, memory_order order)
{
  /* Both MSVC and GCC do the right thing when it's marked volatile */
  return *o;
}
inline ptrdiff_t atomic_exchange_explicit(volatile atomic_ptrdiff_t *o, ptrdiff_t v, memory_order order)
{
  if(sizeof(ptrdiff_t)>4)
    switch(order)
    {
#ifdef InterlockedExchange64Acquire
    case memory_order_acquire:
      return InterlockedExchange64Acquire(o, v);
    case memory_order_release:
      return InterlockedExchange64Release(o, v);
#endif
    default:
      return (ptrdiff_t) InterlockedExchange64((volatile LONGLONG *) o, v);
    }
  else
    switch(order)
    {
#ifdef InterlockedExchangeAcquire
    case memory_order_acquire:
      return InterlockedExchangeAcquire(o, v);
    case memory_order_release:
      return InterlockedExchangeRelease(o, v);
#endif
    default:
      return (ptrdiff_t) InterlockedExchange((volatile long *) o, (long) v);
    }
}
inline ptrdiff_t atomic_fetch_add_explicit(volatile atomic_ptrdiff_t *o, ptrdiff_t v, memory_order order)
{
  if(sizeof(ptrdiff_t)>4)
    switch(order)
    {
#ifdef InterlockedExchangeAdd64Acquire
    case memory_order_acquire:
      return InterlockedExchangeAdd64Acquire(o, v);
    case memory_order_release:
      return InterlockedExchangeAdd64Release(o, v);
#endif
    default:
      return (ptrdiff_t) InterlockedExchangeAdd64((volatile LONGLONG *) o, v);
    }
  else
    switch(order)
    {
#ifdef InterlockedExchangeAddAcquire
    case memory_order_acquire:
      return InterlockedExchangeAddAcquire(o, v);
    case memory_order_release:
      return InterlockedExchangeAddRelease(o, v);
#endif
    default:
      return (ptrdiff_t) InterlockedExchangeAdd((volatile long *) o, (long) v);
    }
}
inline ptrdiff_t atomic_fetch_or_explicit(volatile atomic_ptrdiff_t *o, ptrdiff_t v, memory_order order)
{
  if(sizeof(ptrdiff_t)>4)
    switch(order)
    {
#ifdef InterlockedOr64Acquire
    case memory_order_acquire:
      return InterlockedOr64Acquire((volatile LONGLONG *) o, v);
    case memory_order_release:
      return InterlockedOr64Release((volatile LONGLONG *) o, v);
#endif
    default:
      return (ptrdiff_t) InterlockedOr64((volatile LONGLONG *) o, v);
    }
  else
    switch(order)
    {
#ifdef InterlockedOrAcquire
    case memory_order_acquire:
      return InterlockedOrAcquire((volatile long *) o, (long) v);
    case memory_order_release:
      return InterlockedOrRelease((volatile long *) o, (long) v);
#endif
    default:
      return (ptrdiff_t) InterlockedOr((volatile long *) o, (long) v);
    }
}
#endif
#ifdef __GNUC__
#error Awaiting implementation
#endif




/****************** Declare and define just those bits of threads.h we need ***********************/
enum mtx_types
{
  mtx_plain=0,
  mtx_recursive=1,
  mtx_timed=2
};
enum thrd_results
{
  thrd_success,
  thrd_timeout,
  thrd_busy,
  thrd_error,
  thrd_nomem
};

struct timespec
{
  time_t tv_sec;
  long tv_nsec;
};

#ifdef _MSC_VER
typedef CONDITION_VARIABLE cnd_t;
typedef SRWLOCK mtx_t;

inline int cnd_broadcast(cnd_t *cond) { WakeAllConditionVariable(cond); return thrd_success; }
inline void cnd_destroy(cnd_t *cond) { }
inline int cnd_init(cnd_t *cond) { InitializeConditionVariable(cond); return thrd_success; }
inline int cnd_signal(cnd_t *cond) { WakeConditionVariable(cond); return thrd_success; }
inline int cnd_timedwait(cnd_t *RESTRICT cond, mtx_t *RESTRICT mtx, const struct timespec *RESTRICT ts);
inline int cnd_wait(cnd_t *cond, mtx_t *mtx) { return SleepConditionVariableSRW(cond, mtx, INFINITE, 0) ? thrd_success : thrd_timeout; }

inline void mtx_destroy(mtx_t *mtx) { }
inline int mtx_init(mtx_t *mtx, int type) { InitializeSRWLock(mtx); return thrd_success; }
inline int mtx_lock(mtx_t *mtx) { AcquireSRWLockExclusive(mtx); return thrd_success; }
inline int mtx_timedlock(mtx_t *RESTRICT mtx, const struct timespec *RESTRICT ts);
inline int mtx_trylock(mtx_t *mtx) { return TryAcquireSRWLockExclusive(mtx) ? thrd_success : thrd_busy; }
inline int mtx_unlock(mtx_t *mtx) { ReleaseSRWLockExclusive(mtx); return thrd_success; }

#endif





/****************** Declare and define just those bits of threads.h we need ***********************/
#ifdef _MSC_VER
typedef int (*thrd_start_t)(void *);
typedef uintptr_t thrd_t;

inline int thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
  *thr=_beginthread(func, 0, arg);
  return thrd_success;
}
inline int thrd_sleep(const struct timespec *duration, const struct timespec *remaining)
{
  Sleep((DWORD)(duration->tv_sec*1000+duration->tv_nsec/1000000));
  return thrd_success;
}
#endif

#endif
#endif
