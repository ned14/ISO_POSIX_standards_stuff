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

#define SELECT_PERMITS MAX_PTHREAD_PERMIT_SELECTS
#define _GLIBCXX_ATOMIC_BUILTINS_4 // Without this tries to use std::exception_ptr which Mingw can't handle

#define CATCH_CONFIG_RUNNER
#include "../catch.hpp"
#include <bitset>
#ifdef USE_PARALLEL
#ifdef _MSC_VER
// Use Microsoft's Parallel Patterns Library
#include <ppl.h>
#else
// Use Intel's Threading Building Blocks compatibility layer for the PPL
//#define TBB_USE_CAPTURED_EXCEPTION 1 // Without this tries to use std::exception_ptr which Mingw can't handle
#include "tbb/parallel_for.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/compat/ppl.h"
#endif
#endif

#include "pthread_permit.h"

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

TEST_CASE("pthread_permitX/interchangeable", "Tests that permit1 and permit objects can not be confused by grant")
{
  pthread_permit1_t permit1;
  pthread_permit_t permit;
  REQUIRE(0==pthread_permit1_init(&permit1, 0));
  REQUIRE(0==pthread_permit_init(&permit, 0, 0));

  pthread_permitX_t somepermit;
  somepermit=&permit1;
  REQUIRE(0==pthread_permit1_grant(somepermit));
  somepermit=&permit;
  REQUIRE(0==pthread_permit_grant(somepermit));

  somepermit=&permit;
  REQUIRE(EINVAL==pthread_permit1_grant(somepermit));
  somepermit=&permit1;
  REQUIRE(EINVAL==pthread_permit_grant(somepermit));

  pthread_permit_destroy(&permit);
  REQUIRE(EINVAL==pthread_permit_grant(&permit));
  pthread_permit1_destroy(&permit1);
  REQUIRE(EINVAL==pthread_permit1_grant(&permit1));
}

TEST_CASE("pthread_permit/initdestroy", "Tests repeated init and destroy on same object")
{
  pthread_permit_t permit;
  REQUIRE(0==pthread_permit_init(&permit, 0, 1));
  REQUIRE(0==pthread_permit_grant(&permit));
  pthread_permit_destroy(&permit);
  REQUIRE(EINVAL==pthread_permit_grant(&permit));
  REQUIRE(0==pthread_permit_init(&permit, 0, 1));
  REQUIRE(0==pthread_permit_grant(&permit));
  pthread_permit_destroy(&permit);
  REQUIRE(EINVAL==pthread_permit_grant(&permit));
}

TEST_CASE("pthread_permit/initwait1", "Tests initially granted doesn't wait, and that grants cause exactly one wait")
{
  pthread_permit_t permit;
  REQUIRE(0==pthread_permit_init(&permit, 0, 1));
  REQUIRE(0==pthread_permit_timedwait(&permit, NULL, NULL));
  REQUIRE(ETIMEDOUT==pthread_permit_timedwait(&permit, NULL, NULL));
  pthread_permit_destroy(&permit);
  REQUIRE(EINVAL==pthread_permit_grant(&permit));
}

TEST_CASE("pthread_permit/initwait2", "Tests not initially granted does wait")
{
  pthread_permit_t permit;
  REQUIRE(0==pthread_permit_init(&permit, 0, 0));
  REQUIRE(ETIMEDOUT==pthread_permit_timedwait(&permit, NULL, NULL));
  pthread_permit_destroy(&permit);
  REQUIRE(EINVAL==pthread_permit_grant(&permit));
}

TEST_CASE("pthread_permit/grantwait", "Tests that grants cause exactly one wait")
{
  pthread_permit_t permit;
  REQUIRE(0==pthread_permit_init(&permit, 0, 0));
  REQUIRE(0==pthread_permit_grant(&permit));
  REQUIRE(0==pthread_permit_grant(&permit));
  REQUIRE(0==pthread_permit_timedwait(&permit, NULL, NULL));
  REQUIRE(ETIMEDOUT==pthread_permit_timedwait(&permit, NULL, NULL));
  pthread_permit_destroy(&permit);
  REQUIRE(EINVAL==pthread_permit_grant(&permit));
}

TEST_CASE("pthread_permit/grantrevokewait", "Tests that grants cause exactly one wait and revoke revokes exactly once")
{
  pthread_permit_t permit;
  REQUIRE(0==pthread_permit_init(&permit, 0, 0));
  REQUIRE(ETIMEDOUT==pthread_permit_timedwait(&permit, NULL, NULL));
  REQUIRE(0==pthread_permit_grant(&permit));
  pthread_permit_revoke(&permit);
  REQUIRE(ETIMEDOUT==pthread_permit_timedwait(&permit, NULL, NULL));
  REQUIRE(ETIMEDOUT==pthread_permit_timedwait(&permit, NULL, NULL));

  REQUIRE(0==pthread_permit_grant(&permit));
  REQUIRE(0==pthread_permit_timedwait(&permit, NULL, NULL));
  REQUIRE(ETIMEDOUT==pthread_permit_timedwait(&permit, NULL, NULL));
  pthread_permit_destroy(&permit);
  REQUIRE(EINVAL==pthread_permit_grant(&permit));
}

TEST_CASE("pthread_permit/ncgrantrevokewait", "Tests that non-consuming grants disable all waits")
{
  pthread_permit_t permit;
  REQUIRE(0==pthread_permit_init(&permit, PTHREAD_PERMIT_WAITERS_DONT_CONSUME, 0));
  REQUIRE(ETIMEDOUT==pthread_permit_timedwait(&permit, NULL, NULL));
  REQUIRE(0==pthread_permit_grant(&permit));
  pthread_permit_revoke(&permit);
  REQUIRE(ETIMEDOUT==pthread_permit_timedwait(&permit, NULL, NULL));
  REQUIRE(ETIMEDOUT==pthread_permit_timedwait(&permit, NULL, NULL));

  REQUIRE(0==pthread_permit_grant(&permit));
  REQUIRE(0==pthread_permit_timedwait(&permit, NULL, NULL));
  REQUIRE(0==pthread_permit_timedwait(&permit, NULL, NULL));
  REQUIRE(0==pthread_permit_timedwait(&permit, NULL, NULL));
  pthread_permit_destroy(&permit);
  REQUIRE(EINVAL==pthread_permit_grant(&permit));
}


/***************************** pthread_permit non-parallel/parallel ******************************/

TEST_CASE("pthread_permit/non-parallel/selectfirst", "Tests that select does choose the first available permit exactly once")
{
  pthread_permit_t permits[SELECT_PERMITS];
  size_t n;
  struct timespec ts;
  timespec_get(&ts, TIME_UTC);
  for(n=0; n<SELECT_PERMITS; n++)
  {
    REQUIRE(0==pthread_permit_init(&permits[n], 0, 1));
  }
  for(n=0; n<SELECT_PERMITS; n++)
  {
    size_t m, selectedpermit=(size_t)-1;
    pthread_permit_t *parray[SELECT_PERMITS];
    for(m=0; m<SELECT_PERMITS; m++)
      parray[m]=&permits[m];
    REQUIRE(0==pthread_permit_select(SELECT_PERMITS, parray, NULL, &ts));
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
    REQUIRE(ETIMEDOUT==pthread_permit_timedwait(&permits[selectedpermit], NULL, NULL));
    // Additionally ensure selected item is sequential
    REQUIRE(selectedpermit==n);
  }
  // Make sure nothing permits now
  {
    size_t m;
    pthread_permit_t *parray[SELECT_PERMITS];
    for(m=0; m<SELECT_PERMITS; m++)
      parray[m]=&permits[m];
    REQUIRE(ETIMEDOUT==pthread_permit_select(SELECT_PERMITS, parray, NULL, &ts));
  }
  for(n=0; n<SELECT_PERMITS; n++)
    pthread_permit_destroy(&permits[n]);
}

#ifdef USE_PARALLEL
TEST_CASE("pthread_permit/parallel/selectfirst", "Tests that select does choose the first available permit exactly once")
{
  pthread_permit_t permits[SELECT_PERMITS];
  size_t n;
  struct timespec ts;
  timespec_get(&ts, TIME_UTC);
  for(n=0; n<SELECT_PERMITS; n++)
  {
    REQUIRE(0==pthread_permit_init(&permits[n], 0, 1));
  }
  Concurrency::parallel_for(0, SELECT_PERMITS, [&](size_t n)
  {
    size_t m, selectedpermit=(size_t)-1;
    pthread_permit_t *parray[SELECT_PERMITS];
    for(m=0; m<SELECT_PERMITS; m++)
      parray[m]=&permits[m];
    REQUIRE(0==pthread_permit_select(SELECT_PERMITS, parray, NULL, &ts));
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
    REQUIRE(ETIMEDOUT==pthread_permit_timedwait(&permits[selectedpermit], NULL, NULL));
#if 0
    // Additionally ensure selected item is sequential
    REQUIRE(selectedpermit==n);
#endif
  }
  );
  // Make sure nothing permits now
  {
    size_t m;
    pthread_permit_t *parray[SELECT_PERMITS];
    for(m=0; m<SELECT_PERMITS; m++)
      parray[m]=&permits[m];
    REQUIRE(ETIMEDOUT==pthread_permit_select(SELECT_PERMITS, parray, NULL, &ts));
  }
  for(n=0; n<SELECT_PERMITS; n++)
    pthread_permit_destroy(&permits[n]);
}
#endif

TEST_CASE("pthread_permit/non-parallel/ncselect", "Tests that select does not consume non-consuming permits")
{
  pthread_permit_t permits[SELECT_PERMITS];
  size_t n;
  struct timespec ts;
  timespec_get(&ts, TIME_UTC);
  for(n=0; n<SELECT_PERMITS; n++)
  {
    REQUIRE(0==pthread_permit_init(&permits[n], n==SELECT_PERMITS-1 ? PTHREAD_PERMIT_WAITERS_DONT_CONSUME : 0, 1));
  }
  for(n=0; n<SELECT_PERMITS; n++)
  {
    size_t m, selectedpermit=(size_t)-1;
    pthread_permit_t *parray[SELECT_PERMITS];
    for(m=0; m<SELECT_PERMITS; m++)
      parray[m]=&permits[m];
    REQUIRE(0==pthread_permit_select(SELECT_PERMITS, parray, NULL, &ts));
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
    // Ensure the one selected item won't repermit, except for the NC permit
    REQUIRE((selectedpermit==SELECT_PERMITS-1 ? 0 : ETIMEDOUT)==pthread_permit_timedwait(&permits[selectedpermit], NULL, NULL));
  }
  // Make sure nothing permits now, except for the NC permit
  {
    size_t m;
    pthread_permit_t *parray[SELECT_PERMITS];
    for(m=0; m<SELECT_PERMITS; m++)
      parray[m]=&permits[m];
    REQUIRE(0==pthread_permit_select(SELECT_PERMITS, parray, NULL, &ts));
    for(m=0; m<SELECT_PERMITS; m++)
      REQUIRE(parray[m]==(m==SELECT_PERMITS-1 ? &permits[m] : 0));
  }
  for(n=0; n<SELECT_PERMITS; n++)
    pthread_permit_destroy(&permits[n]);
}

#ifdef USE_PARALLEL
TEST_CASE("pthread_permit/parallel/ncselect", "Tests that select does not consume non-consuming permits")
{
  pthread_permit_t permits[SELECT_PERMITS];
  size_t n;
  struct timespec ts;
  timespec_get(&ts, TIME_UTC);
  for(n=0; n<SELECT_PERMITS; n++)
  {
    REQUIRE(0==pthread_permit_init(&permits[n], n==SELECT_PERMITS-1 ? PTHREAD_PERMIT_WAITERS_DONT_CONSUME : 0, 1));
  }
  Concurrency::parallel_for(0, SELECT_PERMITS, [&](size_t n)
  {
    size_t m, selectedpermit=(size_t)-1;
    pthread_permit_t *parray[SELECT_PERMITS];
    for(m=0; m<SELECT_PERMITS; m++)
      parray[m]=&permits[m];
    REQUIRE(0==pthread_permit_select(SELECT_PERMITS, parray, NULL, &ts));
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
    // Ensure the one selected item won't repermit, except for the NC permit
    REQUIRE((selectedpermit==SELECT_PERMITS-1 ? 0 : ETIMEDOUT)==pthread_permit_timedwait(&permits[selectedpermit], NULL, NULL));
  }
  );
  // Make sure nothing permits now, except for the NC permit
  {
    size_t m;
    pthread_permit_t *parray[SELECT_PERMITS];
    for(m=0; m<SELECT_PERMITS; m++)
      parray[m]=&permits[m];
    REQUIRE(0==pthread_permit_select(SELECT_PERMITS, parray, NULL, &ts));
    for(m=0; m<SELECT_PERMITS; m++)
      REQUIRE(parray[m]==(m==SELECT_PERMITS-1 ? &permits[m] : 0));
  }
  for(n=0; n<SELECT_PERMITS; n++)
    pthread_permit_destroy(&permits[n]);
}
#endif


/***************************** pthread_permit fd mirroring ******************************/

TEST_CASE("pthread_permit/fdmirroring", "Tests that file descriptor mirroring works as intended")
{
  pthread_permit_t permit;
  int fds[2];
  pthread_permit_association_t assoc;
  struct pollfd pfd={0};
  pfd.events=POLLIN;
  REQUIRE(0==pthread_permit_init(&permit, PTHREAD_PERMIT_WAITERS_DONT_CONSUME, 0));
  REQUIRE(0==pipe(fds));
  pfd.fd=fds[0];
  REQUIRE(0!=(assoc=pthread_permit_associate_fd(&permit, fds)));

  REQUIRE(0==poll(&pfd, 1, 0));
  REQUIRE(!(pfd.revents&POLLIN));
  pthread_permit_grant(&permit);
  REQUIRE(0==poll(&pfd, 1, 0));
  REQUIRE(!!(pfd.revents&POLLIN));
  pthread_permit_revoke(&permit);
  REQUIRE(0==poll(&pfd, 1, 0));
  REQUIRE(!(pfd.revents&POLLIN));

  pthread_permit_deassociate(&permit, assoc);
  close(fds[1]); close(fds[0]);
  pthread_permit_destroy(&permit);
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
