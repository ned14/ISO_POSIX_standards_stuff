/* unittests.cpp
Unit testing for pthread_permit1 and pthread_permit
(C) 2012 Niall Douglas http://www.nedproductions.biz/


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

#define SELECT_PERMITS 32
#ifdef __MINGW32__
//#define _GLIBCXX_ATOMIC_BUILTINS_4 // Without this tries to use std::exception_ptr which Mingw can't handle
#endif

#define CATCH_CONFIG_RUNNER
#include "../catch.hpp"
#include <bitset>
#ifdef USE_PARALLEL
#ifdef _MSC_VER
// Use Microsoft's Parallel Patterns Library
#include <ppl.h>
#else
// Use Intel's Threading Building Blocks compatibility layer for the PPL
#ifdef __MINGW32__
#define TBB_USE_CAPTURED_EXCEPTION 1 // Without this tries to use std::exception_ptr which Mingw can't handle
#endif
#include "tbb/parallel_for.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/compat/ppl.h"
#endif
#endif

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#define read _read
#define write _write
#define close _close
#define pipe(fds) _pipe((fds), 4096, _O_BINARY)
  struct pollfd { int fd; short events, revents; };
#define POLLIN 1
#define POLLOUT 2
  // Nasty poll() emulation for Windows
  inline int poll(struct pollfd *fds, size_t nfds, int timeout)
  {
    size_t n, successes=0;
    for(n=0; n<nfds; n++)
    {
      fds[n].revents=0;
      if(fds[n].events&POLLIN)
      {
        // MSVCRT doesn't ask for SYNCHRONIZE permissions in pipe() irritatingly
        //if(WAIT_OBJECT_0==WaitForSingleObject((HANDLE) _get_osfhandle(fds[n].fd), 0)) fds[n].revents|=POLLIN;
        DWORD bytestogo=0;
        PeekNamedPipe((HANDLE) _get_osfhandle(fds[n].fd), NULL, 0, NULL, &bytestogo, NULL);
        if(bytestogo) { fds[n].revents|=POLLIN; successes++; }
      }
      if(fds[n].events&POLLOUT)
      {
        fds[n].revents|=POLLOUT;
        successes++;
      }
    }
    return successes;
  }
#else
#include <unistd.h>
#include <poll.h>
#endif

#include "pthread_permit.h"
#define permitc_init PTHREAD_PERMIT_MANGLEAPI(permitc_init)
#define permitnc_init PTHREAD_PERMIT_MANGLEAPI(permitnc_init)
#define permitc_destroy PTHREAD_PERMIT_MANGLEAPI(permitc_destroy)
#define permitnc_destroy PTHREAD_PERMIT_MANGLEAPI(permitnc_destroy)
#define permitc_grant PTHREAD_PERMIT_MANGLEAPI(permitc_grant)
#define permitnc_grant PTHREAD_PERMIT_MANGLEAPI(permitnc_grant)
#define permitc_revoke PTHREAD_PERMIT_MANGLEAPI(permitc_revoke)
#define permitnc_revoke PTHREAD_PERMIT_MANGLEAPI(permitnc_revoke)
#define permitc_timedwait PTHREAD_PERMIT_MANGLEAPI(permitc_timedwait)
#define permitnc_timedwait PTHREAD_PERMIT_MANGLEAPI(permitnc_timedwait)
#define permit_select PTHREAD_PERMIT_MANGLEAPI(permit_select)
#define permitnc_associate_fd PTHREAD_PERMIT_MANGLEAPI(permitnc_associate_fd)
#define permitnc_deassociate PTHREAD_PERMIT_MANGLEAPI(permitnc_deassociate)

TEST_CASE("timespec/diff", "Tests that timespec_diff works as intended")
{
  struct timespec start, end;
  end.tv_sec=0; end.tv_nsec=500;
  start.tv_sec=0; start.tv_nsec=400;
  REQUIRE(timespec_diff(&end, &start)==100);
  end.tv_sec=0; end.tv_nsec=400;
  start.tv_sec=0; start.tv_nsec=500;
  REQUIRE(timespec_diff(&end, &start)==-100);
  end.tv_sec=1; end.tv_nsec=400;
  start.tv_sec=0; start.tv_nsec=500;
  REQUIRE(timespec_diff(&end, &start)==999999900);
}


/**************************************** pthread_permit1 ****************************************/

TEST_CASE("pthread_permit1/initdestroy", "Tests repeated init and destroy on same object")
{
  pthread_permit1_t permit;
  REQUIRE(0==pthread_permit1_init(&permit, 1));
  REQUIRE(0==pthread_permit1_grant(&permit));
  pthread_permit1_destroy(&permit);
  REQUIRE(EINVAL==pthread_permit1_grant(&permit));
  REQUIRE(0==pthread_permit1_init(&permit, 1));
  REQUIRE(0==pthread_permit1_grant(&permit));
  pthread_permit1_destroy(&permit);
  REQUIRE(EINVAL==pthread_permit1_grant(&permit));
}

TEST_CASE("pthread_permit1/initwait1", "Tests initially granted doesn't wait, and that grants cause exactly one wait")
{
  pthread_permit1_t permit;
  REQUIRE(0==pthread_permit1_init(&permit, 1));
  REQUIRE(0==pthread_permit1_timedwait(&permit, NULL, NULL));
  REQUIRE(ETIMEDOUT==pthread_permit1_timedwait(&permit, NULL, NULL));
  pthread_permit1_destroy(&permit);
  REQUIRE(EINVAL==pthread_permit1_grant(&permit));
}

TEST_CASE("pthread_permit1/initwait2", "Tests not initially granted does wait")
{
  pthread_permit1_t permit;
  REQUIRE(0==pthread_permit1_init(&permit, 0));
  REQUIRE(ETIMEDOUT==pthread_permit1_timedwait(&permit, NULL, NULL));
  pthread_permit1_destroy(&permit);
  REQUIRE(EINVAL==pthread_permit1_grant(&permit));
}

TEST_CASE("pthread_permit1/grantwait", "Tests that grants cause exactly one wait")
{
  pthread_permit1_t permit;
  REQUIRE(0==pthread_permit1_init(&permit, 0));
  REQUIRE(0==pthread_permit1_grant(&permit));
  REQUIRE(0==pthread_permit1_grant(&permit));
  REQUIRE(0==pthread_permit1_timedwait(&permit, NULL, NULL));
  REQUIRE(ETIMEDOUT==pthread_permit1_timedwait(&permit, NULL, NULL));
  pthread_permit1_destroy(&permit);
  REQUIRE(EINVAL==pthread_permit1_grant(&permit));
}

TEST_CASE("pthread_permit1/grantrevokewait", "Tests that grants cause exactly one wait and revoke revokes exactly once")
{
  pthread_permit1_t permit;
  REQUIRE(0==pthread_permit1_init(&permit, 0));
  REQUIRE(ETIMEDOUT==pthread_permit1_timedwait(&permit, NULL, NULL));
  REQUIRE(0==pthread_permit1_grant(&permit));
  pthread_permit1_revoke(&permit);
  REQUIRE(ETIMEDOUT==pthread_permit1_timedwait(&permit, NULL, NULL));
  REQUIRE(ETIMEDOUT==pthread_permit1_timedwait(&permit, NULL, NULL));

  REQUIRE(0==pthread_permit1_grant(&permit));
  REQUIRE(0==pthread_permit1_timedwait(&permit, NULL, NULL));
  REQUIRE(ETIMEDOUT==pthread_permit1_timedwait(&permit, NULL, NULL));
  pthread_permit1_destroy(&permit);
  REQUIRE(EINVAL==pthread_permit1_grant(&permit));
}



/**************************************** pthread_permit ****************************************/

TEST_CASE("pthread_permitX/interchangeable", "Tests that permit1, permitc and permitnc objects can not be confused by grant")
{
  pthread_permit1_t permit1;
  pthread_permitc_t permitc;
  pthread_permitnc_t permitnc;
  REQUIRE(0==pthread_permit1_init(&permit1, 0));
  REQUIRE(0==permitc_init(&permitc, 0));
  REQUIRE(0==permitnc_init(&permitnc, 0));

  pthread_permitX_t somepermit;
  somepermit=&permit1;
  REQUIRE(0==pthread_permit1_grant(somepermit));
  somepermit=&permitc;
  REQUIRE(0==permitc_grant(somepermit));
  somepermit=&permitnc;
  REQUIRE(0==permitnc_grant(somepermit));

  somepermit=&permitc;
  REQUIRE(EINVAL==pthread_permit1_grant(somepermit));
  REQUIRE(EINVAL==permitnc_grant(somepermit));
  somepermit=&permit1;
  REQUIRE(EINVAL==permitc_grant(somepermit));
  REQUIRE(EINVAL==permitnc_grant(somepermit));
  somepermit=&permitnc;
  REQUIRE(EINVAL==pthread_permit1_grant(somepermit));
  REQUIRE(EINVAL==permitc_grant(somepermit));

  permitnc_destroy(&permitnc);
  REQUIRE(EINVAL==permitnc_grant(&permitnc));
  permitc_destroy(&permitc);
  REQUIRE(EINVAL==permitc_grant(&permitc));
  pthread_permit1_destroy(&permit1);
  REQUIRE(EINVAL==pthread_permit1_grant(&permit1));
}

TEST_CASE("pthread_permitc/initdestroy", "Tests repeated init and destroy on same object")
{
  pthread_permitc_t permit;
  REQUIRE(0==permitc_init(&permit, 1));
  REQUIRE(0==permitc_grant(&permit));
  permitc_destroy(&permit);
  REQUIRE(EINVAL==permitc_grant(&permit));
  REQUIRE(0==permitc_init(&permit, 1));
  REQUIRE(0==permitc_grant(&permit));
  permitc_destroy(&permit);
  REQUIRE(EINVAL==permitc_grant(&permit));
}

TEST_CASE("pthread_permitc/initwait1", "Tests initially granted doesn't wait, and that grants cause exactly one wait")
{
  pthread_permitc_t permit;
  REQUIRE(0==permitc_init(&permit, 1));
  REQUIRE(0==permitc_timedwait(&permit, NULL, NULL));
  REQUIRE(ETIMEDOUT==permitc_timedwait(&permit, NULL, NULL));
  permitc_destroy(&permit);
  REQUIRE(EINVAL==permitc_grant(&permit));
}

TEST_CASE("pthread_permitc/initwait2", "Tests not initially granted does wait")
{
  pthread_permitc_t permit;
  REQUIRE(0==permitc_init(&permit, 0));
  REQUIRE(ETIMEDOUT==permitc_timedwait(&permit, NULL, NULL));
  permitc_destroy(&permit);
  REQUIRE(EINVAL==permitc_grant(&permit));
}

TEST_CASE("pthread_permitc/grantwait", "Tests that grants cause exactly one wait")
{
  pthread_permitc_t permit;
  REQUIRE(0==permitc_init(&permit, 0));
  REQUIRE(0==permitc_grant(&permit));
  REQUIRE(0==permitc_grant(&permit));
  REQUIRE(0==permitc_timedwait(&permit, NULL, NULL));
  REQUIRE(ETIMEDOUT==permitc_timedwait(&permit, NULL, NULL));
  permitc_destroy(&permit);
  REQUIRE(EINVAL==permitc_grant(&permit));
}

TEST_CASE("pthread_permitc/grantrevokewait", "Tests that grants cause exactly one wait and revoke revokes exactly once")
{
  pthread_permitc_t permit;
  REQUIRE(0==permitc_init(&permit, 0));
  REQUIRE(ETIMEDOUT==permitc_timedwait(&permit, NULL, NULL));
  REQUIRE(0==permitc_grant(&permit));
  permitc_revoke(&permit);
  REQUIRE(ETIMEDOUT==permitc_timedwait(&permit, NULL, NULL));
  REQUIRE(ETIMEDOUT==permitc_timedwait(&permit, NULL, NULL));

  REQUIRE(0==permitc_grant(&permit));
  REQUIRE(0==permitc_timedwait(&permit, NULL, NULL));
  REQUIRE(ETIMEDOUT==permitc_timedwait(&permit, NULL, NULL));
  permitc_destroy(&permit);
  REQUIRE(EINVAL==permitc_grant(&permit));
}

TEST_CASE("pthread_permitnc/grantrevokewait", "Tests that non-consuming grants disable all waits")
{
  pthread_permitnc_t permit;
  REQUIRE(0==permitnc_init(&permit, 0));
  REQUIRE(ETIMEDOUT==permitnc_timedwait(&permit, NULL, NULL));
  REQUIRE(0==permitnc_grant(&permit));
  permitnc_revoke(&permit);
  REQUIRE(ETIMEDOUT==permitnc_timedwait(&permit, NULL, NULL));
  REQUIRE(ETIMEDOUT==permitnc_timedwait(&permit, NULL, NULL));

  REQUIRE(0==permitnc_grant(&permit));
  REQUIRE(0==permitnc_timedwait(&permit, NULL, NULL));
  REQUIRE(0==permitnc_timedwait(&permit, NULL, NULL));
  REQUIRE(0==permitnc_timedwait(&permit, NULL, NULL));
  permitnc_destroy(&permit);
  REQUIRE(EINVAL==permitnc_grant(&permit));
}


/***************************** pthread_permit non-parallel/parallel ******************************/

TEST_CASE("pthread_permit/non-parallel/selectfirst", "Tests that select does choose the first available permit exactly once")
{
  pthread_permitc_t permits[SELECT_PERMITS];
  size_t n;
  struct timespec ts;
  timespec_get(&ts, TIME_UTC);
  for(n=0; n<SELECT_PERMITS; n++)
  {
    REQUIRE(0==permitc_init(&permits[n], 1));
  }
  for(n=0; n<SELECT_PERMITS; n++)
  {
    size_t m, selectedpermit=(size_t)-1;
    pthread_permitX_t parray[SELECT_PERMITS];
    for(m=0; m<SELECT_PERMITS; m++)
      parray[m]=&permits[m];
    REQUIRE(0==permit_select(SELECT_PERMITS, parray, NULL, &ts));
    // Ensure exactly one item has been selected
    for(m=0; m<SELECT_PERMITS; m++)
    {
      if((size_t)-1==selectedpermit && parray[m])
      {
        REQUIRE(parray[m]==&permits[m]);
        selectedpermit=m;
      }
      else
        REQUIRE(parray[m]==0);
    }
    // Ensure the one selected item won't repermit
    REQUIRE(ETIMEDOUT==permitc_timedwait(&permits[selectedpermit], NULL, NULL));
    // Additionally ensure selected item is sequential
    REQUIRE(selectedpermit==n);
  }
  // Make sure nothing permits now
  {
    size_t m;
    pthread_permitX_t parray[SELECT_PERMITS];
    for(m=0; m<SELECT_PERMITS; m++)
      parray[m]=&permits[m];
    REQUIRE(ETIMEDOUT==permit_select(SELECT_PERMITS, parray, NULL, &ts));
  }
  for(n=0; n<SELECT_PERMITS; n++)
    permitc_destroy(&permits[n]);
}

#ifdef USE_PARALLEL
TEST_CASE("pthread_permit/parallel/selectfirst", "Tests that select does choose the first available permit exactly once")
{
  pthread_permitc_t permits[SELECT_PERMITS];
  size_t n;
  struct timespec ts;
  atomic_uint permitted;
  permitted=0;

  timespec_get(&ts, TIME_UTC);
  for(n=0; n<SELECT_PERMITS; n++)
  {
    REQUIRE(0==permitc_init(&permits[n], 1));
  }
  Concurrency::parallel_for(0, SELECT_PERMITS, [&permits, ts, &permitted](size_t n)
  {
    size_t m, selectedpermit=(size_t)-1;
    pthread_permitX_t parray[SELECT_PERMITS];
    int ret;
    for(m=0; m<SELECT_PERMITS; m++)
      parray[m]=&permits[m];
    if((0!=(ret=permit_select(SELECT_PERMITS, parray, NULL, &ts))))
      REQUIRE(0==ret);
    // Ensure exactly one item has been selected
    for(m=0; m<SELECT_PERMITS; m++)
    {
      if((size_t)-1==selectedpermit && parray[m])
      {
        if(parray[m]!=&permits[m])
          REQUIRE(parray[m]==&permits[m]);
        selectedpermit=m;
      }
      else if(parray[m]!=0)
        REQUIRE(parray[m]==0);
    }
    // Ensure the one selected item won't repermit
    if(ETIMEDOUT!=(ret=permitc_timedwait(&permits[selectedpermit], NULL, NULL)))
      REQUIRE(ETIMEDOUT==ret);
#if 0
    // Additionally ensure selected item is sequential
    if(selectedpermit!=n)
      REQUIRE(selectedpermit==n);
#else
    atomic_fetch_add_explicit(&permitted, 1U, memory_order_relaxed);
#endif
  }
  );
  // Make sure nothing permits now
  REQUIRE((atomic_load_explicit(&permitted, memory_order_relaxed))==SELECT_PERMITS);
  {
    size_t m;
    pthread_permitX_t parray[SELECT_PERMITS];
    for(m=0; m<SELECT_PERMITS; m++)
      parray[m]=&permits[m];
    REQUIRE(ETIMEDOUT==permit_select(SELECT_PERMITS, parray, NULL, &ts));
  }
  for(n=0; n<SELECT_PERMITS; n++)
    permitc_destroy(&permits[n]);
}
#endif

TEST_CASE("pthread_permit/non-parallel/ncselect", "Tests that select does not consume non-consuming permits")
{
  pthread_permitc_t permitcs[SELECT_PERMITS-1];
  pthread_permitnc_t permitnc;
  size_t n;
  struct timespec ts;
  timespec_get(&ts, TIME_UTC);
  for(n=0; n<SELECT_PERMITS-1; n++)
  {
    REQUIRE(0==permitc_init(&permitcs[n], 1));
  }
  REQUIRE(0==permitnc_init(&permitnc, 1));
  for(n=0; n<SELECT_PERMITS; n++)
  {
    size_t m, selectedpermit=(size_t)-1;
    pthread_permitX_t parray[SELECT_PERMITS];
    for(m=0; m<SELECT_PERMITS-1; m++)
      parray[m]=&permitcs[m];
    parray[m]=&permitnc;
    REQUIRE(0==permit_select(SELECT_PERMITS, parray, NULL, &ts));
    // Ensure exactly one item has been selected
    for(m=0; m<SELECT_PERMITS; m++)
    {
      if((size_t)-1==selectedpermit && parray[m])
      {
        REQUIRE((m==SELECT_PERMITS-1 || parray[m]==&permitcs[m]));
        selectedpermit=m;
      }
      else
        REQUIRE(parray[m]==0);
    }
    // Ensure the one selected item won't repermit, except for the NC permit
    if(selectedpermit==SELECT_PERMITS-1)
      REQUIRE(0==permitnc_timedwait(&permitnc, NULL, NULL));
    else
      REQUIRE(ETIMEDOUT==permitc_timedwait(&permitcs[selectedpermit], NULL, NULL));
  }
  // Make sure nothing permits now, except for the NC permit
  {
    size_t m;
    pthread_permitX_t parray[SELECT_PERMITS];
    for(m=0; m<SELECT_PERMITS-1; m++)
      parray[m]=&permitcs[m];
    parray[m]=&permitnc;
    REQUIRE(0==permit_select(SELECT_PERMITS, parray, NULL, &ts));
    for(m=0; m<SELECT_PERMITS; m++)
      REQUIRE(parray[m]==(m==SELECT_PERMITS-1 ? &permitnc : 0));
  }
  for(n=0; n<SELECT_PERMITS-1; n++)
    permitc_destroy(&permitcs[n]);
  permitnc_destroy(&permitnc);
}

#ifdef USE_PARALLEL
TEST_CASE("pthread_permit/parallel/ncselect", "Tests that select does not consume non-consuming permits")
{
  pthread_permitc_t permitcs[SELECT_PERMITS-1];
  pthread_permitnc_t permitnc;
  size_t n;
  struct timespec ts;
  atomic_uint permitted;
  permitted=0;

  timespec_get(&ts, TIME_UTC);
  for(n=0; n<SELECT_PERMITS-1; n++)
  {
    REQUIRE(0==permitc_init(&permitcs[n], 1));
  }
  REQUIRE(0==permitnc_init(&permitnc, 1));
  Concurrency::parallel_for(0, SELECT_PERMITS, [&permitcs, &permitnc, ts, &permitted](size_t n)
  {
    size_t m, selectedpermit=(size_t)-1;
    pthread_permitX_t parray[SELECT_PERMITS];
    int ret;
    for(m=0; m<SELECT_PERMITS-1; m++)
      parray[m]=&permitcs[m];
    parray[m]=&permitnc;
    if(0!=(ret=permit_select(SELECT_PERMITS, parray, NULL, &ts)))
      REQUIRE(0==ret);
    // Ensure exactly one item has been selected
    for(m=0; m<SELECT_PERMITS; m++)
    {
      if((size_t)-1==selectedpermit && parray[m])
      {
        if((m!=SELECT_PERMITS-1 && parray[m]!=&permitcs[m]))
          REQUIRE((m==SELECT_PERMITS-1 || parray[m]==&permitcs[m]));
        selectedpermit=m;
      }
      else if(parray[m]!=0)
        REQUIRE(parray[m]==0);
    }
    atomic_fetch_add_explicit(&permitted, 1U, memory_order_relaxed);
    // Ensure the one selected item won't repermit, except for the NC permit
    if(selectedpermit==SELECT_PERMITS-1)
    {
      if(0!=(ret=permitnc_timedwait(&permitnc, NULL, NULL)))
        REQUIRE(0==ret);
    }
    else
    {
      if(ETIMEDOUT!=(ret=permitc_timedwait(&permitcs[selectedpermit], NULL, NULL)))
        REQUIRE(ETIMEDOUT==ret);
    }
  }
  );
  // Make sure nothing permits now, except for the NC permit
  REQUIRE((atomic_load_explicit(&permitted, memory_order_relaxed))==SELECT_PERMITS);
  {
    size_t m;
    pthread_permitX_t parray[SELECT_PERMITS];
    for(m=0; m<SELECT_PERMITS-1; m++)
      parray[m]=&permitcs[m];
    parray[m]=&permitnc;
    REQUIRE(0==permit_select(SELECT_PERMITS, parray, NULL, &ts));
    for(m=0; m<SELECT_PERMITS; m++)
      REQUIRE(parray[m]==(m==SELECT_PERMITS-1 ? &permitnc : 0));
  }
  for(n=0; n<SELECT_PERMITS-1; n++)
    permitc_destroy(&permitcs[n]);
  permitnc_destroy(&permitnc);
}
#endif


/***************************** pthread_permit fd mirroring ******************************/

TEST_CASE("pthread_permit/fdmirroring", "Tests that file descriptor mirroring works as intended")
{
  pthread_permitnc_t permit;
  int fds[2];
  pthread_permitnc_association_t assoc;
  struct pollfd pfd={0};
  pfd.events=POLLIN;
  REQUIRE(0==permitnc_init(&permit, 0));
  REQUIRE(0==pipe(fds));
  pfd.fd=fds[0];
  REQUIRE(0!=(assoc=permitnc_associate_fd(&permit, fds)));

  REQUIRE(poll(&pfd, 1, 0)>=0);
  REQUIRE(!(pfd.revents&POLLIN));
  permitnc_grant(&permit);
  REQUIRE(poll(&pfd, 1, 0)>=0);
  REQUIRE(!!(pfd.revents&POLLIN));
  permitnc_revoke(&permit);
  REQUIRE(poll(&pfd, 1, 0)>=0);
  REQUIRE(!(pfd.revents&POLLIN));

  permitnc_deassociate(&permit, assoc);
  close(fds[1]); close(fds[0]);
  permitnc_destroy(&permit);
}



int main(int argc, char *argv[])
{
#ifdef USE_PARALLEL
  size_t threads=(size_t)-1;
#ifdef _MSC_VER
  {
    size_t n;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION slpi[256];
    DWORD len=sizeof(slpi);
    GetLogicalProcessorInformation(slpi, &len);
    assert(ERROR_INSUFFICIENT_BUFFER!=GetLastError());
    threads=0;
    for(n=0; n<len/sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); n++)
    {
      if(RelationProcessorCore==slpi[n].Relationship) threads+=std::bitset<64>((unsigned long long) slpi[n].ProcessorMask).count();
    }
  }
#else
  threads=tbb::task_scheduler_init::default_num_threads();
#endif
  printf("These unit tests have been compiled with parallel support. I will use %d threads.\n", threads);
#else
  printf("These unit tests have not been compiled with parallel support and will execute only those which are sequential.\n");
#endif
  int result=Catch::Main(argc, argv);
  return result;
}
