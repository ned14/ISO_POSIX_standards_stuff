/* test_permit.c
Tests the proposed C1X permit object
(C) 2011 Niall Douglas http://www.nedproductions.biz/


On a 2.67Ghz Intel Core 2 Quad:

DoConsume:

Uncontended grant time: 61 cycles
Uncontended revoke time: 45 cycles
Uncontended wait time: 135 cycles

1 contended grant time: 731 cycles
1 contended revoke time: 160 cycles
1 contended wait time: 1195 cycles


DontConsume:

Uncontended grant time: 102 cycles
Uncontended revoke time: 46 cycles
Uncontended wait time: 137 cycles

1 contended grant time: 856 cycles
1 contended revoke time: 207 cycles
1 contended wait time: 887 cycles
*/

#include "c1x_notifier.h"
#include "timing.h"
#include <stdio.h>

#define THREADS 2
#define CYCLESPERMICROSECOND (2.67*1000000000/1000000000000)
#define DONTCONSUME 1
// Define to test uncontended, set to what to exclude
#define UNCONTENDED 0

static usCount timingoverhead;
static thrd_t threads[THREADS];
static volatile int done;
static permit_t permit;

void sleep(long ms)
{
  struct timespec ts;
  ts.tv_sec=ms/1000;
  ts.tv_nsec=(ms % 1000)*1000000;
  thrd_sleep(&ts, NULL);
}

int threadfunc(void *mynum)
{
  size_t mythread=(size_t) mynum;
  usCount start, end;
  size_t count=0;
#ifdef UNCONTENDED
  if(UNCONTENDED==mythread) return 0;
#endif
  if(!mynum)
  {
    usCount revoketotal=0, granttotal=0;
    while(!done)
    {
      // Revoke permit
      start=GetUsCount();
      permit_revoke(&permit);
      end=GetUsCount();
      revoketotal+=end-start-timingoverhead;
      //printf("Thread %u revoked permit\n", mythread);
      //sleep(1000);
      //printf("\nThread %u granting permit\n", mythread);
      start=GetUsCount();
      permit_grant(&permit);
      end=GetUsCount();
      granttotal+=end-start-timingoverhead;
      //sleep(1);
      count++;
    }
    printf("Thread %u, average revoke/grant time was %u/%u cycles\n", mythread, (size_t)((double)revoketotal/count*CYCLESPERMICROSECOND), (size_t)((double)granttotal/count*CYCLESPERMICROSECOND));
    permit_grant(&permit);
  }
  else
  {
    usCount waittotal=0;
    mtx_t mtx;
    mtx_init(&mtx, mtx_plain);
    mtx_lock(&mtx);
    while(!done)
    {
      // Wait on permit
      start=GetUsCount();
      permit_wait(&permit, &mtx);
      end=GetUsCount();
      waittotal+=end-start-timingoverhead;
      count++;
      //printf("%u", mythread);
#if defined(UNCONTENDED) && 0==DONTCONSUME
      if(UNCONTENDED==0 && 1==mythread)
      {
        permit_grant(&permit);
      }
#endif
    }
    printf("Thread %u, average wait time was %u cycles\n", mythread, (size_t)((double)waittotal/count*CYCLESPERMICROSECOND));
  }
  return 0;
}

int main(void)
{
  int n;
  usCount start;
  permit_init(&permit, DONTCONSUME, 1);
  printf("Press key to continue ...\n");
  getchar();
  printf("Wait ...\n");
  start=GetUsCount();
  while(GetUsCount()-start<3000000000000ULL);
  printf("Calculating timing overhead ...\n");
  for(n=0; n<5000000; n++)
  {
    start=GetUsCount();
    timingoverhead+=GetUsCount()-start;
  }
  timingoverhead/=5000000;
  printf("Timing overhead on this machine is %u us. Go!\n", timingoverhead);
  for(n=0; n<THREADS; n++)
  {
    thrd_create(&threads[n], threadfunc, (void *)(size_t)n);
  }
  printf("Press key to kill all\n");
  getchar();
  done=1;
  sleep(2000);
  printf("Press key to exit\n");
  getchar();
  return 0;
}