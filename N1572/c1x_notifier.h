/* c1x_notifier.h
Declares and defines the proposed C1X semaphore object
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

#ifndef C1X_NOTIFIER_H
#define C1X_NOTIFIER_H
#include "c1x_compat.h"
#include <assert.h>

/* Define the permit_t type */
typedef struct permit_s
{
  int replacePermit;                  /* What to replace the permit with when consumed */
  atomic_uint permit;                 /* =0 no permit, =1 yes permit */
  atomic_uint waiters, waited;        /* Keeps track of when a thread waits and wakes */
  cnd_t cond;                         /* Wakes anything waiting for a permit */
} permit_t;

/* Creates with whether waiters don't consume and initial state. */
inline int permit_init(permit_t *permit, _Bool waitersDontConsume, _Bool initial);

/* Destroys a permit */
inline void permit_destroy(permit_t *permit);

/* Grants permit to one or all waiting threads. */
inline int permit_grant(permit_t *permit);

/* Revoke any outstanding permit, causing any subsequent waiters to wait. */
inline void permit_revoke(permit_t *permit);

/* Waits for permit to become available, atomically unlocking the specified mutex when waiting.
If mtx is NULL, never sleeps instead looping forever waiting for permit. */
inline int permit_wait(permit_t *permit, mtx_t *mtx);

/* Waits for a time for a permit to become available, atomically unlocking the specified mutex when waiting. */
inline int permit_timedwait(permit_t *permit, mtx_t *mtx, const struct timespec *ts);




inline int permit_init(permit_t *permit, _Bool waitersDontConsume, _Bool initial)
{
  permit->replacePermit=waitersDontConsume;
  permit->permit=initial;
  permit->waiters=permit->waited=0;
  if(thrd_success!=cnd_init(&permit->cond)) return thrd_error;
  return thrd_success;
}

inline void permit_destroy(permit_t *permit)
{
  permit->replacePermit=1;
  permit->permit=1;
  cnd_destroy(&permit->cond);
}

inline int permit_grant(permit_t *permit)
{ // Grant permit
  atomic_store_explicit(&permit->permit, 1, memory_order_seq_cst);
  // Are there waiters on the permit?
  if(atomic_load_explicit(&permit->waiters, memory_order_relaxed)!=atomic_load_explicit(&permit->waited, memory_order_relaxed))
  { // There are indeed waiters. If waiters don't consume permits, release everything
    if(permit->replacePermit)
    { // Loop waking until nothing is waiting
      do
      {
        if(thrd_success!=cnd_broadcast(&permit->cond)) return thrd_error;
        thrd_yield();
      } while(atomic_load_explicit(&permit->waiters, memory_order_relaxed)!=atomic_load_explicit(&permit->waited, memory_order_relaxed));
    }
    else
    { // Loop waking until at least one thread takes the permit
      while(atomic_load_explicit(&permit->permit, memory_order_relaxed))
      {
        if(thrd_success!=cnd_signal(&permit->cond)) return thrd_error;
        thrd_yield();
      }
    }
  }
  return thrd_success;
}

inline void permit_revoke(permit_t *permit)
{
  atomic_store_explicit(&permit->permit, 0, memory_order_relaxed);
}

inline int permit_wait(permit_t *permit, mtx_t *mtx)
{
  int expected, ret=thrd_success;
  // Increment the monotonic count to indicate we have entered a wait
  atomic_fetch_add_explicit(&permit->waiters, 1, memory_order_acquire);
  // Fetch me a permit, excluding all other threads if replacePermit is zero
  while((expected=1, !atomic_compare_exchange_weak_explicit(&permit->permit, &expected, permit->replacePermit, memory_order_relaxed, memory_order_relaxed)))
  { // Permit is not granted, so wait if we have a mutex
    if(mtx)
    {
      if(thrd_success!=cnd_wait(&permit->cond, mtx)) ret=thrd_error;
    }
  }
  // Increment the monotonic count to indicate we have exited a wait
  atomic_fetch_add_explicit(&permit->waited, 1, memory_order_relaxed);
  return ret;
}


#endif
