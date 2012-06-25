/* test_sema2.c
Tests the proposed C1X as a semaphore
(C) 2011 Niall Douglas http://www.nedproductions.biz/

On my 2.67Ghz Intel Core 2 Quad running Windows 7 x64:

Thread 2, average lock/unlock time was 637/1012 cycles
Thread 3, average lock/unlock time was 678/1064 cycles
Thread 1, average lock/unlock time was 658/1060 cycles
Thread 0, average lock/unlock time was 6541/7913 cycles

Contended: 658/1045
Uncontended: 42/116

*/

#include "c1x_sema.h"
#include "timing.h"
#include <stdio.h>

#define THREADS 4
#define CYCLESPERMICROSECOND (2.67*1000000000/1000000000000)

static usCount timingoverhead;
static thrd_t threads[THREADS];
static int done;
static sema_t readers, writers;

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
  usCount start, end, locktotal=0, unlocktotal=0;
  size_t count=0;
  //if(mythread!=1) return 0;
  while(!done)
  {
    if(!mynum)
    {
      // To lock for writing
      start=GetUsCount();
      sema_incr(&writers, -1);    // This decrements the count, stopping any new readers
      sema_wait(&readers, 0);     // This waits until the readers are done
      end=GetUsCount();
      locktotal+=end-start-timingoverhead;
      //printf("Started write, readers=%d, writers=%d\n", sema_incr(&readers, 0), sema_incr(&writers, 0));
      //sleep(100);
      start=GetUsCount();
      sema_set(&writers, 1);      // This releases the readers
      end=GetUsCount();
      unlocktotal+=end-start-timingoverhead;
      sleep(50);
    }
    else
    {
      // To lock for reading
      start=GetUsCount();
      if(sema_incr(&writers, 0)<1) sema_wait(&writers, -1); // Wait if something wants to write
      sema_incr(&readers, -1);    // Marks me as reading
      end=GetUsCount();
      locktotal+=end-start-timingoverhead;
      start=GetUsCount();
      sema_incr(&readers, 1);     // Marks me as done reading
      end=GetUsCount();
      unlocktotal+=end-start-timingoverhead;
    }
    count++;
  }
  printf("Thread %u, average lock/unlock time was %u/%u cycles\n", mythread, (size_t)((double)locktotal/count*CYCLESPERMICROSECOND), (size_t)((double)unlocktotal/count*CYCLESPERMICROSECOND));
  return 0;
}

int main(void)
{
  int n;
  usCount start;
  sema_create(&readers, 0);   // Permit no writers while there are readers
  sema_create(&writers, 1);   // Permit one writer through at once
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
  printf("readers=%d, writers=%d\n", sema_incr(&readers, 0), sema_incr(&writers, 0));
  printf("Press key to exit\n");
  getchar();
  return 0;
}