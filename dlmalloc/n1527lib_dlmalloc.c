/* n1527lib_dlmalloc.c
A modified dlmalloc implementation of the N1527 proposal for the C programming language
(C) 2010-2011 Niall Douglas http://www.nedproductions.biz/


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

#include "../n1527lib.h"
#include <string.h>
#include <errno.h>
#include <malloc.h>

#define ONLY_MSPACES 1
/*#define DEBUG 1*/
#include "malloc.c.h"
static mspace syspool;

static void init_syspool(void)
{
  ensure_initialization();
  ACQUIRE_MALLOC_GLOBAL_LOCK();
  if(!syspool)
  {
    if(!(syspool=create_mspace(0, 1))) abort();
  }
  RELEASE_MALLOC_GLOBAL_LOCK();
}

N1527MALLOCNOALIASATTR N1527MALLOCPTRATTR void *n1527_aligned_alloc(size_t alignment, size_t size)
{
  if(!syspool) init_syspool();
  return mspace_malloc2(syspool, size, alignment, 0, 0);
}

N1527MALLOCNOALIASATTR N1527MALLOCPTRATTR void *n1527_aligned_realloc(void *ptr, size_t alignment, size_t size)
{
  if(!syspool) init_syspool();
  return mspace_realloc2(syspool, ptr, size, alignment, 0, 0);
}

N1527MALLOCNOALIASATTR N1527MALLOCPTRATTR void **n1527_batch_alloc1(int *errnos, void **ptrs, size_t *RESTRICT count, size_t *RESTRICT size, size_t alignment, size_t reserve, uintmax_t flags)
{
  int n;
  size_t maxn=*count, _count=0;
  if(!ptrs)
  {
    if(!(ptrs=(void **) n1527_calloc(maxn, sizeof(void *))))
    {
      *count=0;
      return NULL;
    }
    flags|=M2_BATCH_IS_ALL_ALLOC;
  }
  if(!size || !(*size))
  {
    for(n=0; n<(int) maxn; n++)
    {
      if(ptrs[n])
      {
        mspace_free2(syspool, ptrs[n], (unsigned) flags);
        ptrs[n]=0;
      }
      _count++;
    }
  }
  else
  {
    size_t _size;
    {
      void *temp=mspace_malloc2(syspool, *size, alignment, reserve, (unsigned) flags);
      *size=_size=mspace_usable_size(temp);
      mspace_free2(syspool, temp, (unsigned) flags);
    }
    if((flags & M2_BATCH_IS_ALL_ALLOC) && !alignment && !reserve)
    {
      int opts=1/*all same size*/;
      if(flags & M2_ZERO_MEMORY) opts|=2/*zero memory*/;
      ialloc((mstate) syspool, maxn, size, opts, ptrs);
      _count=maxn;
    }
    else for(n=0; n<(int) maxn; n++)
    {
      void *temp=0;
      if(ptrs[n]) /* resize */
      {
        if(!(flags & M2_PREVENT_MOVE))
          temp=mspace_realloc2(syspool, ptrs[n], _size, alignment, reserve, (unsigned) flags);
      }
      else /* allocate */
      {
        temp=mspace_malloc2(syspool, _size, alignment, reserve, (unsigned) flags);
      }
      if(temp) /* success */
      {
        ptrs[n]=temp;
        if(errnos) errnos[n]=0;
        _count++;
      }
      else /* failure */
      {
        if(errnos) errnos[n]=(ptrs[n] && (flags & M2_PREVENT_MOVE)) ? ENOSPC : ENOMEM;
      }
    }
  }
  *count=_count;
  return ptrs;
}

_Bool n1527_batch_alloc2(int *errnos, struct n1527_mallocation2 **RESTRICT mdataptrs, size_t *RESTRICT count, size_t alignment, size_t reserve, uintmax_t flags)
{
  int n;
  size_t maxn=*count, _count=0;
  if((flags & M2_BATCH_IS_ALL_ALLOC) && !alignment && !reserve)
  {
    void **ptrs=(void **) mspace_malloc(syspool, maxn*sizeof(void *));
    size_t *sizes=(size_t *) mspace_malloc(syspool, maxn*sizeof(size_t));
    int opts=0;
    if(flags & M2_ZERO_MEMORY) opts|=2/*zero memory*/;
    for(n=0; n<(int) maxn; n++)
      sizes[n]=mdataptrs[n]->size;
    ialloc((mstate) syspool, maxn, sizes, opts, ptrs);
    for(n=0; n<(int) maxn; n++)
      mdataptrs[n]->ptr=ptrs[n];
    mspace_free(syspool, ptrs);
    mspace_free(syspool, sizes);
    _count=maxn;
  }
  else for(n=0; n<(int) maxn; n++)
  {
    if(!mdataptrs[n] || !mdataptrs[n]->size) /* free */
    {
      if(mdataptrs[n] && mdataptrs[n]->ptr)
      {
        mspace_free2(syspool, mdataptrs[n]->ptr, (unsigned) flags);
        mdataptrs[n]->ptr=0;
      }
      _count++;
    }
    else
    {
      void *temp=0;
      if(mdataptrs[n]->ptr) /* resize */
      {
        if(!(flags & M2_PREVENT_MOVE))
          temp=mspace_realloc2(syspool, mdataptrs[n]->ptr, mdataptrs[n]->size, alignment, reserve, (unsigned) flags);
      }
      else /* allocate */
      {
        temp=mspace_malloc2(syspool, mdataptrs[n]->size, alignment, reserve, (unsigned) flags);
      }
      if(temp) /* success */
      {
        mdataptrs[n]->ptr=temp;
        mdataptrs[n]->size=mspace_usable_size(temp);
        if(errnos) errnos[n]=0;
        _count++;
      }
      else /* failure */
      {
        if(errnos) errnos[n]=(mdataptrs[n]->ptr && (flags & M2_PREVENT_MOVE)) ? ENOSPC : ENOMEM;
      }
    }
  }
  *count=_count;
  return _count==maxn;
}

_Bool n1527_batch_alloc5(int *errnos, struct n1527_mallocation5 **RESTRICT mdataptrs, size_t *RESTRICT count)
{
  int n;
  size_t maxn=*count, _count=0;
  for(n=0; n<(int) maxn; n++)
  {
    if(!mdataptrs[n] || !mdataptrs[n]->size) /* free */
    {
      if(mdataptrs[n] && mdataptrs[n]->ptr)
      {
        mspace_free2(syspool, mdataptrs[n]->ptr, (unsigned) mdataptrs[n]->flags);
        mdataptrs[n]->ptr=0;
      }
      _count++;
    }
    else
    {
      void *temp=0;
      if(mdataptrs[n]->ptr) /* resize */
      {
        if(!(mdataptrs[n]->flags & M2_PREVENT_MOVE))
          temp=mspace_realloc2(syspool, mdataptrs[n]->ptr, mdataptrs[n]->size, mdataptrs[n]->alignment, mdataptrs[n]->reserve, (unsigned) mdataptrs[n]->flags);
      }
      else /* allocate */
      {
        temp=mspace_malloc2(syspool, mdataptrs[n]->size, mdataptrs[n]->alignment, mdataptrs[n]->reserve, (unsigned) mdataptrs[n]->flags);
      }
      if(temp) /* success */
      {
        mdataptrs[n]->ptr=temp;
        mdataptrs[n]->size=mspace_usable_size(temp);
        if(errnos) errnos[n]=0;
        _count++;
      }
      else /* failure */
      {
        if(errnos) errnos[n]=(mdataptrs[n]->ptr && (mdataptrs[n]->flags & M2_PREVENT_MOVE)) ? ENOSPC : ENOMEM;
      }
    }
  }
  *count=_count;
  return _count==maxn;
}

N1527MALLOCNOALIASATTR N1527MALLOCPTRATTR void *n1527_calloc(size_t nmemb, size_t size)
{
  if((size_t)-1/size<nmemb) { return 0; }
  if(!syspool) init_syspool();
  return mspace_malloc2(syspool, nmemb*size, 0, 0, M2_ZERO_MEMORY);
}

void n1527_free(void *ptr)
{
  if(!syspool) init_syspool();
  mspace_free2(syspool, ptr, 0);
}

N1527MALLOCNOALIASATTR N1527MALLOCPTRATTR void *n1527_malloc(size_t size)
{
  if(!syspool) init_syspool();
  return mspace_malloc2(syspool, size, 0, 0, 0);
}

size_t n1527_malloc_usable_size(void *ptr)
{
  return mspace_usable_size(ptr);
}

N1527MALLOCNOALIASATTR N1527MALLOCPTRATTR void *n1527_realloc(void *ptr, size_t size)
{
  if(!syspool) init_syspool();
  return mspace_realloc2(syspool, ptr, size, 0, 0, 0);
}

N1527MALLOCNOALIASATTR N1527MALLOCPTRATTR void *n1527_try_aligned_realloc(void *ptr, size_t alignment, size_t size)
{
  if(!syspool) init_syspool();
  return mspace_realloc2(syspool, ptr, size, alignment, 0, M2_PREVENT_MOVE);
}

N1527MALLOCNOALIASATTR N1527MALLOCPTRATTR void *n1527_try_realloc(void *ptr, size_t size)
{
  if(!syspool) init_syspool();
  return mspace_realloc2(syspool, ptr, size, 0, 0, M2_PREVENT_MOVE);
}


