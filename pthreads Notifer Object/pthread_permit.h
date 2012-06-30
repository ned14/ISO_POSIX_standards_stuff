/* pthread_permit.h
Declares and defines the proposed C1X semaphore object
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

#ifndef PTHREAD_PERMIT_H
#define PTHREAD_PERMIT_H
#include "pthread_permit1.h"
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#define read _read
#define write _write
#define pipe(fds) _pipe((fds), 4096, _O_BINARY)
extern "C" {
  struct pollfd { int fd; short events, revents; };
#define POLLIN 1
#define POLLOUT 2
  // Nasty poll() emulation for Windows
  inline int poll(struct pollfd *fds, size_t nfds, int timeout)
  {
    size_t n;
    for(n=0; n<nfds; n++)
    {
      fds[n].revents=0;
      if(fds[n].events&POLLIN)
      {
        // MSVCRT doesn't ask for SYNCHRONIZE permissions in pipe() irritatingly
        //if(WAIT_OBJECT_0==WaitForSingleObject((HANDLE) _get_osfhandle(fds[n].fd), 0)) fds[n].revents|=POLLIN;
        DWORD bytestogo=0;
        PeekNamedPipe((HANDLE) _get_osfhandle(fds[n].fd), NULL, 0, NULL, &bytestogo, NULL);
        if(bytestogo) fds[n].revents|=POLLIN;
      }
    }
    return 0;
  }
}
#else
#include <unistd.h>
#include <poll.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*! Define the pthread_permit_t type */
typedef struct pthread_permit_s pthread_permit_t;
/*! Define the pthread_permit_hook_t_np type */
typedef struct pthread_permit_hook_s pthread_permit_hook_t_np;
typedef struct pthread_permit_hook_s
{
  int (*func)(pthread_permit_t *permit, pthread_permit_hook_t_np *hookdata);
  void *data;
  pthread_permit_hook_t_np *next;
} pthread_permit_hook_t_np;
/*! Define the pthread_permit_association_t type */
typedef struct pthread_permit_association_s *pthread_permit_association_t;
/*! Flag indicating waiters don't consume a permit */
#define PTHREAD_PERMIT_WAITERS_DONT_CONSUME 1
/*! Type of permit hook routine */
typedef enum pthread_permit_hook_type_np
{
  PTHREAD_PERMIT_HOOK_TYPE_DESTROY,
  PTHREAD_PERMIT_HOOK_TYPE_GRANT,
  PTHREAD_PERMIT_HOOK_TYPE_REVOKE,
  PTHREAD_PERMIT_HOOK_TYPE_WAIT,

  PTHREAD_PERMIT_HOOK_TYPE_LAST
} pthread_permit_hook_type_t_np;

/*! Creates a permit with initial state. */
inline int pthread_permit_init(pthread_permit_t *permit, size_t flags, _Bool initial);

/*! Pushes a hook routine which is called when a permit changes state [NON-PORTABLE]. You
almost certainly want to allocate \em hook statically. hook->next is set to the previous
hook setting - it is up to you whether to call it in your hook routine however.
*/
inline int pthread_permit_pushhook_np(pthread_permit_t *permit, pthread_permit_hook_type_t_np type, pthread_permit_hook_t_np *hook);

/*! Pops a hook routine which is called when a permit changes state [NON-PORTABLE]. Simply
sets the current hook to hook->next, returning the former hook.
*/
inline pthread_permit_hook_t_np *pthread_permit_pophook_np(pthread_permit_t *permit, pthread_permit_hook_type_t_np type);

/*! Destroys a permit */
inline void pthread_permit_destroy(pthread_permit_t *permit);

/*! Grants permit to all currently waiting threads. If there is no waiting thread, permits the next waiting thread. */
inline int pthread_permit_grant(pthread_permitX_t permit);

/*! Revoke any outstanding permit, causing any subsequent waiters to wait. */
inline void pthread_permit_revoke(pthread_permit_t *permit);

/*! Waits for permit to become available, atomically unlocking the specified mutex when waiting.
If mtx is NULL, never sleeps instead looping forever waiting for permit. */
inline int pthread_permit_wait(pthread_permit_t *permit, pthread_mutex_t *mtx);

/*! Waits for a time for a permit to become available, atomically unlocking the specified mutex when waiting.
If mtx is NULL, never sleeps instead looping forever waiting for permit. If ts is NULL,
returns immediately instead of waiting.

Returns: 0: success; EINVAL: bad permit, mutex or timespec; ETIMEDOUT: the time period specified by ts expired.
*/
inline int pthread_permit_timedwait(pthread_permit_t *permit, pthread_mutex_t *mtx, const struct timespec *ts);

/*! Waits for a time for any permit in the supplied list of permits to become available, 
atomically unlocking the specified mutex when waiting. If mtx is NULL, never sleeps instead
looping forever waiting for a permit. If ts is NULL, waits forever.

On exit, the permits array has all ungranted permits zeroed, or if there is an error then
only errored permits are zeroed. Only the first granted permit is ever returned so if no error then
only one element of the array will be set on return, but if error then many elements can be returned.

The complexity of this call is O(no). If we could use dynamic memory we could achieve O(1).

Returns: 0: success; EINVAL: bad permit, mutex or timespec; ETIMEDOUT: the time period specified by ts expired.
*/
inline int pthread_permit_select(size_t no, pthread_permit_t **RESTRICT permits, pthread_mutex_t *mtx, const struct timespec *ts);

/*! Non-consuming permits only. Sets a file descriptor whose signalled state should match
the permit's state i.e. the descriptor has a single byte written to it to make it signalled
when the permit is granted. When the permit is revoked, the descriptor is repeatedly read
from until it is non-signalled. Note that this mechanism uses the non-portable hook system
but it does upcall previously installed hooks. Note that this function uses malloc().

Note that this call is not thread safe. Note you must call pthread_permit_deassociate() on the
returned value before destroying its associated permit.
*/
inline pthread_permit_association_t pthread_permit_associate_fd(pthread_permit_t *permit, int fds[2]);

/*! Non-consuming permits only. Removes a previous file descriptor association.

Note that this call is not thread safe.
*/
inline void pthread_permit_deassociate(pthread_permit_t *permit, pthread_permit_association_t assoc);

#ifdef _WIN32
/*! Non-consuming permits only. Sets a windows HANDLE whose signalled state should match
the permit's state i.e. the handle has a single byte written to it to make it signalled
when the permit is granted. When the permit is revoked, the handle is repeatedly read
from until it is non-signalled. Note that this mechanism uses the non-portable hook system
but it does upcall previously installed hooks. Note that this function uses malloc().

Note that this call is not thread safe.
*/
inline pthread_permit_association_t pthread_permit_associate_handle_np(pthread_permit_t *permit, HANDLE h);
#endif


//! The maximum number of pthread_permit_select which can occur simultaneously
#define MAX_PTHREAD_PERMIT_SELECTS 64
typedef struct pthread_permit_select_s
{
  atomic_uint magic;                  /* Used to ensure this structure is valid */
  cnd_t cond;                         /* Wakes anything waiting for a permit */
} pthread_permit_select_t;
typedef struct pthread_permit_s
{ /* NOTE: KEEP THIS HEADER THE SAME AS pthread_permit1_t to allow its grant() to optionally work here */
  atomic_uint magic;                  /* Used to ensure this structure is valid */
  atomic_uint permit;                 /* =0 no permit, =1 yes permit */
  atomic_uint waiters, waited;        /* Keeps track of when a thread waits and wakes */
  cnd_t cond;                         /* Wakes anything waiting for a permit */

  /* Extensions from pthread_permit1_t type */
  unsigned replacePermit;             /* What to replace the permit with when consumed */
  atomic_uint lockWake;               /* Used to exclude new wakers if and only if waiters don't consume */
  pthread_permit_hook_t_np *RESTRICT hooks[PTHREAD_PERMIT_HOOK_TYPE_LAST];
  pthread_permit_select_t *volatile RESTRICT selects[MAX_PTHREAD_PERMIT_SELECTS]; /* select permit parent */
} pthread_permit_t;
struct pthread_permit_association_s
{
  struct pthread_permit_hook_s grant, revoke;
};
extern pthread_permit_select_t pthread_permit_selects[MAX_PTHREAD_PERMIT_SELECTS];
#if 1 // for testing
pthread_permit_select_t pthread_permit_selects[MAX_PTHREAD_PERMIT_SELECTS];
#endif // testing

int pthread_permit_init(pthread_permit_t *permit, unsigned flags, _Bool initial)
{
  memset(permit, 0, sizeof(pthread_permit_t));
  permit->permit=initial;
  if(thrd_success!=cnd_init(&permit->cond)) return thrd_error;
  permit->replacePermit=(flags&PTHREAD_PERMIT_WAITERS_DONT_CONSUME)!=0;
  atomic_store_explicit(&permit->magic, *(const unsigned *)"PPER", memory_order_seq_cst);
  return thrd_success;
}

int pthread_permit_pushhook_np(pthread_permit_t *permit, pthread_permit_hook_type_t_np type, pthread_permit_hook_t_np *hook)
{
  unsigned expected;
  if(*(const unsigned *)"PPER"!=permit->magic || type<0 || type>=PTHREAD_PERMIT_HOOK_TYPE_LAST) return thrd_error;
  // Serialise
  while((expected=0, !atomic_compare_exchange_weak_explicit(&permit->lockWake, &expected, 1U, memory_order_relaxed, memory_order_relaxed)))
  {
    //if(1==cpus) thrd_yield();
  }
  hook->next=permit->hooks[type];
  permit->hooks[type]=hook;
  permit->lockWake=0;
  return thrd_success;
}

pthread_permit_hook_t_np *pthread_permit_pophook_np(pthread_permit_t *permit, pthread_permit_hook_type_t_np type)
{
  unsigned expected;
  pthread_permit_hook_t_np *ret;
  if(*(const unsigned *)"PPER"!=permit->magic || type<0 || type>=PTHREAD_PERMIT_HOOK_TYPE_LAST) { return (pthread_permit_hook_t_np *)(size_t)-1; }
  // Serialise
  while((expected=0, !atomic_compare_exchange_weak_explicit(&permit->lockWake, &expected, 1U, memory_order_relaxed, memory_order_relaxed)))
  {
    //if(1==cpus) thrd_yield();
  }
  ret=permit->hooks[type];
  permit->hooks[type]=ret->next;
  permit->lockWake=0;
  return ret;
}

void pthread_permit_destroy(pthread_permit_t *permit)
{
  if(*(const unsigned *)"PPER"!=permit->magic) return;
  if(permit->hooks[PTHREAD_PERMIT_HOOK_TYPE_DESTROY])
    permit->hooks[PTHREAD_PERMIT_HOOK_TYPE_DESTROY]->func(permit, permit->hooks[PTHREAD_PERMIT_HOOK_TYPE_DESTROY]);
  /* Mark this object as invalid for further use */
  atomic_store_explicit(&permit->magic, 0U, memory_order_seq_cst);
  permit->replacePermit=1;
  permit->permit=1;
  cnd_destroy(&permit->cond);
}

int pthread_permit_grant(pthread_permitX_t _permit)
{ // If permits aren't consumed, prevent any new waiters or granters
  pthread_permit_t *permit=(pthread_permit_t *) _permit;
  int ret=thrd_success;
  size_t n;
  if(*(const unsigned *)"PPER"!=permit->magic) return thrd_error;
  if(permit->replacePermit)
  {
    unsigned expected;
    // Only one grant may occur concurrently if permits aren't consumed
    while((expected=0, !atomic_compare_exchange_weak_explicit(&permit->lockWake, &expected, 1U, memory_order_relaxed, memory_order_relaxed)))
    {
      //if(1==cpus) thrd_yield();
    }
  }
  // Grant permit
  atomic_store_explicit(&permit->permit, 1U, memory_order_seq_cst);
  if(permit->hooks[PTHREAD_PERMIT_HOOK_TYPE_GRANT])
    permit->hooks[PTHREAD_PERMIT_HOOK_TYPE_GRANT]->func(permit, permit->hooks[PTHREAD_PERMIT_HOOK_TYPE_GRANT]);
  // Are there waiters on the permit?
  if(atomic_load_explicit(&permit->waiters, memory_order_relaxed)!=atomic_load_explicit(&permit->waited, memory_order_relaxed))
  { // There are indeed waiters. If waiters don't consume permits, release everything
    if(permit->replacePermit)
    { // Loop waking until nothing is waiting
      do
      {
        if(thrd_success!=cnd_broadcast(&permit->cond))
        {
          ret=thrd_error;
          goto exit;
        }
        // Are there select operations on the permit?
        for(n=0; n<MAX_PTHREAD_PERMIT_SELECTS; n++)
        {
          if(permit->selects[n])
          {
            if(thrd_success!=cnd_signal(&permit->selects[n]->cond))
            {
              ret=thrd_error;
              goto exit;
            }
          }
        }
        //if(1==cpus) thrd_yield();
      } while(atomic_load_explicit(&permit->waiters, memory_order_relaxed)!=atomic_load_explicit(&permit->waited, memory_order_relaxed));
    }
    else
    { // Loop waking until at least one thread takes the permit
      while(atomic_load_explicit(&permit->permit, memory_order_relaxed))
      {
        if(thrd_success!=cnd_signal(&permit->cond))
        {
          ret=thrd_error;
          goto exit;
        }
        // Are there select operations on the permit?
        for(n=0; n<MAX_PTHREAD_PERMIT_SELECTS; n++)
        {
          if(permit->selects[n])
          {
            if(thrd_success!=cnd_signal(&permit->selects[n]->cond))
            {
              ret=thrd_error;
              goto exit;
            }
          }
        }
        //if(1==cpus) thrd_yield();
      }
    }
  }
exit:
  // If permits aren't consumed, granting has completed, so permit new waiters and granters
  if(permit->replacePermit)
    permit->lockWake=0;
  return ret;
}

void pthread_permit_revoke(pthread_permit_t *permit)
{
  if(*(const unsigned *)"PPER"!=permit->magic) return;
  atomic_store_explicit(&permit->permit, 0U, memory_order_relaxed);
  if(permit->hooks[PTHREAD_PERMIT_HOOK_TYPE_REVOKE])
    permit->hooks[PTHREAD_PERMIT_HOOK_TYPE_REVOKE]->func(permit, permit->hooks[PTHREAD_PERMIT_HOOK_TYPE_REVOKE]);
}

int pthread_permit_wait(pthread_permit_t *permit, pthread_mutex_t *mtx)
{
  int ret=thrd_success;
  unsigned expected;
  if(*(const unsigned *)"PPER"!=permit->magic) return thrd_error;
  // If permits aren't consumed, if a permit is executing then wait here
  if(permit->replacePermit)
  {
    while(atomic_load_explicit(&permit->lockWake, memory_order_acquire))
    {
      //if(1==cpus) thrd_yield();
    }
  }
  // Increment the monotonic count to indicate we have entered a wait
  atomic_fetch_add_explicit(&permit->waiters, 1U, memory_order_acquire);
  // Fetch me a permit, excluding all other threads if replacePermit is zero
  while((expected=1, !atomic_compare_exchange_weak_explicit(&permit->permit, &expected, permit->replacePermit, memory_order_relaxed, memory_order_relaxed)))
  { // Permit is not granted, so wait if we have a mutex
    if(mtx)
    {
      if(thrd_success!=cnd_wait(&permit->cond, mtx)) ret=thrd_error;
    }
    else thrd_yield();
  }
  // Increment the monotonic count to indicate we have exited a wait
  atomic_fetch_add_explicit(&permit->waited, 1U, memory_order_relaxed);
  return ret;
}

int pthread_permit_timedwait(pthread_permit_t *permit, pthread_mutex_t *mtx, const struct timespec *ts)
{
  int ret=thrd_success;
  unsigned expected;
  struct timespec now;
  if(*(const unsigned *)"PPER"!=permit->magic) return thrd_error;
  // If permits aren't consumed, if a permit is executing then wait here
  if(permit->replacePermit)
  {
    while(atomic_load_explicit(&permit->lockWake, memory_order_acquire))
    {
      //if(1==cpus) thrd_yield();
    }
  }
  // Increment the monotonic count to indicate we have entered a wait
  atomic_fetch_add_explicit(&permit->waiters, 1U, memory_order_acquire);
  // Fetch me a permit, excluding all other threads if replacePermit is zero
  while((expected=1, !atomic_compare_exchange_weak_explicit(&permit->permit, &expected, permit->replacePermit, memory_order_relaxed, memory_order_relaxed)))
  { // Permit is not granted, so wait if we have a mutex
    if(!ts) { ret=thrd_timeout; break; }
    else
    {
      timespec_get(&now, TIME_UTC);
      long long diff=timespec_diff(ts, &now);
      if(diff<=0) { ret=thrd_timeout; break; }
    }
    if(mtx)
    {
      int cndret=cnd_timedwait(&permit->cond, mtx, ts);
      if(thrd_success!=cndret && thrd_timeout!=cndret) { ret=cndret; break; }
    }
    else thrd_yield();
  }
  // Increment the monotonic count to indicate we have exited a wait
  atomic_fetch_add_explicit(&permit->waited, 1U, memory_order_relaxed);
  return ret;
}

int pthread_permit_select(size_t no, pthread_permit_t **RESTRICT permits, pthread_mutex_t *mtx, const struct timespec *ts)
{
  int ret=thrd_success;
  unsigned expected;
  struct timespec now;
  pthread_permit_select_t *myselect=0;
  size_t n, replacePermits=0, selectslot=(size_t)-1, selectedpermit=(size_t)-1;
  // Sanity check permits
  for(n=0; n<no; n++)
  {
    if(*(const unsigned *)"PPER"!=permits[n]->magic)
    {
      permits[n]=0;
      if(thrd_success!=ret) ret=thrd_error;
    }
    if(permits[n]->replacePermit) replacePermits++;
  }
  if(thrd_success!=ret) return ret;
  // Find a free slot for us to use
  for(n=0; n<MAX_PTHREAD_PERMIT_SELECTS; n++)
  {
    expected=0;
    if(atomic_compare_exchange_weak_explicit(&pthread_permit_selects[n].magic, &expected, *(const unsigned *)"SPER", memory_order_relaxed, memory_order_relaxed))
    {
      selectslot=n;
      break;
    }
  }
  if(MAX_PTHREAD_PERMIT_SELECTS==n) return thrd_nomem;
  myselect=&pthread_permit_selects[selectslot];
  if(thrd_success!=(ret=cnd_init(&myselect->cond))) return ret;

  // Link each of the permits into our select slot
  for(n=0; n<no; n++)
  {
    // If the permit isn't consumed, if the permit is executing then wait here
    if(permits[n]->replacePermit)
    {
      while(atomic_load_explicit(&permits[n]->lockWake, memory_order_acquire))
      {
        //if(1==cpus) thrd_yield();
      }
      replacePermits--;
    }
    // Increment the monotonic count to indicate we have entered a wait
    atomic_fetch_add_explicit(&permits[n]->waiters, 1U, memory_order_acquire);
    // Set the select
    assert(!permits[n]->selects[selectslot]);
    permits[n]->selects[selectslot]=myselect;
  }
  assert(!replacePermits);

  // Loop the permits, trying to grab a permit
  for(;;)
  {
    for(n=0; n<no; n++)
    {
      expected=1;
      if(atomic_compare_exchange_weak_explicit(&permits[n]->permit, &expected, permits[n]->replacePermit, memory_order_relaxed, memory_order_relaxed))
      { // Permit is granted
        selectedpermit=n;
        break;
      }
    }
    if((size_t)-1!=selectedpermit) break;
    // Permit is not granted, so wait if we have a mutex
    if(ts)
    {
      timespec_get(&now, TIME_UTC);
      long long diff=timespec_diff(ts, &now);
      if(diff<=0) { ret=thrd_timeout; break; }
    }
    if(mtx)
    {
      int cndret=(ts ? cnd_timedwait(&myselect->cond, mtx, ts) : cnd_wait(&myselect->cond, mtx));
      if(thrd_success!=cndret && thrd_timeout!=cndret) { ret=cndret; break; }
    }
    else thrd_yield();
  }

  // Delink each of the permits from our select slot
  for(n=0; n<no; n++)
  {
    // Unset the select
    assert(permits[n]->selects[selectslot]==myselect);
    permits[n]->selects[selectslot]=0;
    // Increment the monotonic count to indicate we have exited a wait
    atomic_fetch_add_explicit(&permits[n]->waited, 1U, memory_order_relaxed);
    // Zero if not selected
    if(selectedpermit!=n) permits[n]=0;
  }
  // Destroy the select slot's condition variable and reset
  cnd_destroy(&myselect->cond);
  myselect->magic=0;
  return ret;
}

int pthread_permit_associate_fd_hook_grant(pthread_permit_t *permit, pthread_permit_hook_t_np *hookdata)
{
  int fd=(int)(size_t)(hookdata->data);
  char buffer=0;
  struct pollfd pfd={0};
  pfd.fd=fd;
  pfd.events=POLLOUT;
  poll(&pfd, 1, 0);
  if(!(pfd.revents&POLLOUT))
    write(fd, &buffer, 1);
  return hookdata->next ? hookdata->next->func(permit, hookdata->next) : 0;
}
int pthread_permit_associate_fd_hook_revoke(pthread_permit_t *permit, pthread_permit_hook_t_np *hookdata)
{
  int fd=(int)(size_t)(hookdata->data);
  char buffer[256];
  struct pollfd pfd={0};
  pfd.fd=fd;
  pfd.events=POLLIN;
  do
  {
    pfd.revents=0;
    poll(&pfd, 1, 0);
    if(pfd.revents&POLLIN)
      read(fd, buffer, 256);
  } while(pfd.revents&POLLIN);
  return hookdata->next ? hookdata->next->func(permit, hookdata->next) : 0;
}
pthread_permit_association_t pthread_permit_associate_fd(pthread_permit_t *permit, int fds[2])
{
  pthread_permit_association_t ret;
  if(*(const unsigned *)"PPER"!=permit->magic || !permit->replacePermit) return 0;
  ret=(pthread_permit_association_t) calloc(1, sizeof(struct pthread_permit_association_s));
  if(!ret) return ret;
  ret->grant.func=pthread_permit_associate_fd_hook_grant;
  ret->revoke.func=pthread_permit_associate_fd_hook_revoke;
  ret->revoke.data=(void *)(size_t) fds[0]; // Revoke reads until empty
  ret->grant.data=(void *)(size_t) fds[1];  // Grant writes a single byte
  if(thrd_success!=pthread_permit_pushhook_np(permit, PTHREAD_PERMIT_HOOK_TYPE_GRANT, &ret->grant))
  {
    free(ret);
    return 0;
  }
  if(thrd_success!=pthread_permit_pushhook_np(permit, PTHREAD_PERMIT_HOOK_TYPE_REVOKE, &ret->revoke))
  {
    pthread_permit_pophook_np(permit, PTHREAD_PERMIT_HOOK_TYPE_GRANT);
    free(ret);
    return 0;
  }
  if(permit->permit)
  {
    char buffer=0;
    write(fds[1], &buffer, 1);
  }
  return ret;
}

void pthread_permit_deassociate(pthread_permit_t *permit, pthread_permit_association_t assoc)
{
  pthread_permit_hook_t_np *RESTRICT *hookptr;
  if(*(const unsigned *)"PPER"!=permit->magic || !permit->replacePermit) return;
  for(hookptr=&permit->hooks[PTHREAD_PERMIT_HOOK_TYPE_GRANT]; *hookptr; hookptr=&(*hookptr)->next)
  {
    if(*hookptr==&assoc->grant)
    {
      *hookptr=(*hookptr)->next;
      break;
    }
  }
  for(hookptr=&permit->hooks[PTHREAD_PERMIT_HOOK_TYPE_REVOKE]; *hookptr; hookptr=&(*hookptr)->next)
  {
    if(*hookptr==&assoc->revoke)
    {
      *hookptr=(*hookptr)->next;
      break;
    }
  }
  free(assoc);
}

#ifdef _WIN32
int pthread_permit_associate_handle_hook_grant(pthread_permit_t *permit, pthread_permit_hook_t_np *hookdata)
{
  HANDLE h=hookdata->data;
  char buffer=0;
  DWORD written=0;
  if(WAIT_TIMEOUT==WaitForSingleObject(h, 0))
    WriteFile(h, &buffer, 1, &written, NULL);
  return hookdata->next ? hookdata->next->func(permit, hookdata->next) : 0;
}
int pthread_permit_associate_handle_hook_revoke(pthread_permit_t *permit, pthread_permit_hook_t_np *hookdata)
{
  HANDLE h=hookdata->data;
  char buffer[256];
  DWORD read=0;
  while(WAIT_OBJECT_0==WaitForSingleObject(h, 0))
    ReadFile(h, buffer, 256, &read, NULL);
  return hookdata->next ? hookdata->next->func(permit, hookdata->next) : 0;
}
pthread_permit_association_t pthread_permit_associate_handle_np(pthread_permit_t *permit, HANDLE h)
{
  pthread_permit_association_t ret;
  if(*(const unsigned *)"PPER"!=permit->magic || !permit->replacePermit) return 0;
  ret=(pthread_permit_association_t) calloc(1, sizeof(struct pthread_permit_association_s));
  if(!ret) return ret;
  ret->grant.func=pthread_permit_associate_handle_hook_grant;
  ret->revoke.func=pthread_permit_associate_handle_hook_revoke;
  ret->grant.data=ret->revoke.data=h;
  if(thrd_success!=pthread_permit_pushhook_np(permit, PTHREAD_PERMIT_HOOK_TYPE_GRANT, &ret->grant))
  {
    free(ret);
    return 0;
  }
  if(thrd_success!=pthread_permit_pushhook_np(permit, PTHREAD_PERMIT_HOOK_TYPE_REVOKE, &ret->revoke))
  {
    pthread_permit_pophook_np(permit, PTHREAD_PERMIT_HOOK_TYPE_GRANT);
    free(ret);
    return 0;
  }
  if(permit->permit)
  {
    char buffer=0;
    DWORD written=0;
    WriteFile(h, &buffer, 1, &written, NULL);
  }
  return ret;
}
#endif

#ifdef __cplusplus
}
#endif

#endif
