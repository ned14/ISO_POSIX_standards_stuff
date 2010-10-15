/* n1527lib_test.c
Tests the N1527 proposal for the C programming language implementations
(C) 2010 Niall Douglas http://www.nedproductions.biz/


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

#define RECORDS 4000000

#include "n1527lib.h"
#include <stdio.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
typedef unsigned __int64 usCount;
static usCount GetUsCount()
{
	static LARGE_INTEGER ticksPerSec;
	static double scalefactor;
	LARGE_INTEGER val;
	if(!scalefactor)
	{
		if(QueryPerformanceFrequency(&ticksPerSec))
			scalefactor=ticksPerSec.QuadPart/1000000000000.0;
		else
			scalefactor=1;
	}
	if(!QueryPerformanceCounter(&val))
		return (usCount) GetTickCount() * 1000000000;
	return (usCount) (val.QuadPart/scalefactor);
}
#else
#include <sys/time.h>
#include <time.h>
typedef unsigned long long usCount;
static usCount GetUsCount()
{
#ifdef CLOCK_MONOTONIC
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ((usCount) ts.tv_sec*1000000000000LL)+ts.tv_nsec*1000LL;
#else
	struct timeval tv;
	gettimeofday(&tv, 0);
	return ((usCount) tv.tv_sec*1000000000000LL)+tv.tv_usec*1000000LL;
#endif
}
#endif

static size_t sizes[RECORDS];
static void *ptrs[RECORDS];
static struct n1527_mallocation2 mdata[RECORDS], *mdataptrs[RECORDS];

int main(void)
{
  size_t n, m;
  usCount start, end;
  printf("N1527lib test program\n"
         "-=-=-=-=-=-=-=-=-=-=-\n");
  srand(1);
  for(n=0; n<RECORDS; n++)
    sizes[n]=rand() & 1023;
  {
    unsigned frees=0;
    printf("Fragmenting free space ...\n");
    start=GetUsCount();
    while(GetUsCount()-start<3000000000000)
    {
      n=rand() % RECORDS;
      if(ptrs[n]) { n1527_free(ptrs[n]); ptrs[n]=0; frees++; }
      else ptrs[n]=n1527_malloc(sizes[n]);
    }
    memset(ptrs, 0, RECORDS*sizeof(void *));
    printf("Did %u frees\n", frees);
  }

  if(1)
  {
    printf("\nBatch single sized test\n"
             "-----------------------\n");
    for(m=0; m<2; m++)
    {
      //break;
      start=GetUsCount();
      for(n=0; n<RECORDS; n++)
      {
        ptrs[n]=n1527_malloc(1024);
      }
      end=GetUsCount();
      printf("malloc() does %f mallocs/sec\n\n", RECORDS/((end-start)/1000000000000.0));

      start=GetUsCount();
      for(n=0; n<RECORDS; n++)
      {
        n1527_free(ptrs[n]);
        ptrs[n]=0;
      }
      end=GetUsCount();
      printf("free() does %f frees/sec\n\n", RECORDS/((end-start)/1000000000000.0));
    }

    {
      size_t count=RECORDS, size=1024;
      start=GetUsCount();
      n1527_batch_alloc1(NULL, ptrs, &count, &size, 0, 0, M2_BATCH_IS_ALL_ALLOC);
      end=GetUsCount();
      printf("batch_alloc1(M2_BATCH_IS_ALL_ALLOC) does %f mallocs/sec\n\n", RECORDS/((end-start)/1000000000000.0));

      count=RECORDS;
      start=GetUsCount();
      n1527_batch_alloc1(NULL, ptrs, &count, NULL, 0, 0, 0);
      end=GetUsCount();
      printf("batch_alloc1() does %f frees/sec\n\n", RECORDS/((end-start)/1000000000000.0));
    }
    {
      size_t count=RECORDS, size=1024;
      start=GetUsCount();
      n1527_batch_alloc1(NULL, ptrs, &count, &size, 0, 0, 0);
      end=GetUsCount();
      printf("batch_alloc1() does %f mallocs/sec\n\n", RECORDS/((end-start)/1000000000000.0));

      count=RECORDS;
      start=GetUsCount();
      n1527_batch_alloc1(NULL, ptrs, &count, NULL, 0, 0, M2_BATCH_IS_ALL_FREE|M2_CONSTANT_TIME);
      end=GetUsCount();
      printf("batch_alloc1(M2_CONSTANT_TIME) does %f frees/sec\n\n", RECORDS/((end-start)/1000000000000.0));
    }
  }
  if(1)
  {
    printf("\nBatch multiple sized test\n"
             "-------------------------\n");
    for(m=0; m<2; m++)
    {
      start=GetUsCount();
      for(n=0; n<RECORDS; n++)
      {
        if(!(ptrs[n]=n1527_malloc(sizes[n]))) abort();
        /*if(!((n+1) & 65535)) printf("%u\n", n);*/
      }
      end=GetUsCount();
      printf("malloc() does %f mallocs/sec\n\n", RECORDS/((end-start)/1000000000000.0));

      start=GetUsCount();
      for(n=0; n<RECORDS; n++)
      {
        n1527_free(ptrs[n]);
        ptrs[n]=0;
        /*if(!((n+1) & 131071)) printf(".");*/
      }
      end=GetUsCount();
      printf("free() does %f frees/sec\n\n", RECORDS/((end-start)/1000000000000.0));
    }

    {
      size_t count=RECORDS;
      for(n=0; n<RECORDS; n++)
      {
        mdataptrs[n]=mdata+n;
        mdata[n].ptr=0;
        mdata[n].size=sizes[n];
      }
      start=GetUsCount();
      n1527_batch_alloc2(NULL, mdataptrs, &count, 0, 0, M2_BATCH_IS_ALL_ALLOC);
      end=GetUsCount();
      printf("batch_alloc2(M2_BATCH_IS_ALL_ALLOC) does %f mallocs/sec\n\n", RECORDS/((end-start)/1000000000000.0));

      count=RECORDS;
      for(n=0; n<RECORDS; n++)
        mdata[n].size=0;
      start=GetUsCount();
      n1527_batch_alloc2(NULL, mdataptrs, &count, 0, 0, 0);
      end=GetUsCount();
      printf("batch_alloc2() does %f frees/sec\n\n", RECORDS/((end-start)/1000000000000.0));
    }

    {
      size_t count=RECORDS;
      for(n=0; n<RECORDS; n++)
      {
        mdataptrs[n]=mdata+n;
        mdata[n].ptr=0;
        mdata[n].size=sizes[n];
      }
      start=GetUsCount();
      n1527_batch_alloc2(NULL, mdataptrs, &count, 0, 0, 0);
      end=GetUsCount();
      printf("batch_alloc2() does %f mallocs/sec\n\n", RECORDS/((end-start)/1000000000000.0));

      count=RECORDS;
      for(n=0; n<RECORDS; n++)
        mdata[n].size=0;
      start=GetUsCount();
      n1527_batch_alloc2(NULL, mdataptrs, &count, 0, 0, M2_BATCH_IS_ALL_FREE|M2_CONSTANT_TIME);
      end=GetUsCount();
      printf("batch_alloc2(M2_CONSTANT_TIME) does %f frees/sec\n\n", RECORDS/((end-start)/1000000000000.0));
    }
  }
#ifdef _MSC_VER
  printf("Press Return to exit ...\n");
  getchar();
#endif
  return 0;
}
