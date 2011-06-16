/* test_sema.c
Tests the proposed C1X as a semaphore
(C) 2011 Niall Douglas http://www.nedproductions.biz/
*/

#include "c1x_sema.h"
#include <stdio.h>

#define THREADS 1000

static thrd_t threads[THREADS];
static int done;
static sema_t sema;

int threadfunc(void *mynum)
{
  size_t mythread=(size_t) mynum;
  while(!done)
  {
    sema_wait(&sema, -1);
    if(!done) printf("Thread %u woken\n", mythread);
  }
  if(!done) printf("Thread %u exiting\n", mythread);
  return 0;
}

void sleep(long ms)
{
  struct timespec ts;
  ts.tv_sec=ms/1000;
  ts.tv_nsec=(ms % 1000)*1000000;
  thrd_sleep(&ts, NULL);
}

int main(void)
{
  int n;
  sema_create(&sema, 0);
  for(n=0; n<THREADS; n++)
  {
    thrd_create(&threads[n], threadfunc, (void *)(size_t)n);
  }
  printf("Semaphore count=%d\n", sema_incr(&sema, 0));
  printf("Semaphore increment(5)=%d\n", sema_incr(&sema, 5));
  sleep(500);
  printf("Semaphore increment(25)=%d\n", sema_incr(&sema, 25));
  printf("Semaphore increment(3)=%d\n", sema_incr(&sema, 3));
  sleep(500);
  printf("Semaphore count=%d\n", sema_incr(&sema, 0));
  printf("Press key to kill all but 750\n");
  getchar();
  done=1;
  printf("Semaphore set(-750)=%d\n", sema_set(&sema, -750));
  printf("Press key to kill all\n");
  getchar();
  printf("Semaphore count=%d\n", sema_set(&sema, 0));
  sleep(500);
  printf("Semaphore count=%d\n", sema_incr(&sema, 0));
  printf("Press key to exit\n");
  getchar();
  return 0;
}