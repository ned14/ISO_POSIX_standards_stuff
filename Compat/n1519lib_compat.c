/* n1519lib_compat.c
A C99 compatible implementation of the N1519 proposal for the C programming language
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

#include "../n1519lib.h"
#include <string.h>
#include <errno.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include <malloc.h>

N1519MALLOCNOALIASATTR N1519MALLOCPTRATTR void *aligned_alloc(size_t alignment, size_t size)
{
#ifdef _MSC_VER
  return _aligned_malloc(size, alignment);
#else
  return memalign(alignment, size);
#endif
}

N1519MALLOCNOALIASATTR N1519MALLOCPTRATTR void *aligned_realloc(void *ptr, size_t alignment, size_t size)
{
#ifdef _MSC_VER
  return _aligned_realloc(ptr, size, alignment);
#else
  /* Sadly no standard POSIX choice here */
  void *temp;
  size_t oldsize=malloc_usable_size(ptr);
  if(!(temp=memalign(alignment, size))) return 0;
  memcpy(temp, ptr, (oldsize<size) ? oldsize : size);
  free(ptr);
  return temp;
#endif
}

N1519MALLOCNOALIASATTR N1519MALLOCPTRATTR void **batch_alloc1(int *errnos, void **ptrs, size_t *RESTRICT count, size_t *RESTRICT size, size_t alignment, size_t reserve, uintmax_t flags)
{
  int n;
  size_t maxn=*count, _count=0;
  if(!ptrs && !(ptrs=(void **) calloc(maxn, sizeof(void *))))
  {
    *count=0;
    return NULL;
  }
  if(!size || !(*size))
  {
#ifdef _OPENMP
#pragma omp parallel for reduction(+:_count) schedule(guided)
#endif
    for(n=0; n<(int) maxn; n++)
    {
      if(ptrs[n])
      {
#ifdef _MSC_VER
        msvcfree(ptrs[n]);
#else
        free(ptrs[n]);
#endif
        ptrs[n]=0;
      }
      _count++;
    }
  }
  else
  {
    size_t _size;
    {
      void *temp=alignment ? aligned_alloc(alignment, *size) : malloc(*size);
      *size=_size=malloc_usable_size(temp);
#ifdef _MSC_VER
      msvcfree(temp);
#else
      free(temp);
#endif
    }
#ifdef _OPENMP
#pragma omp parallel for reduction(+:_count) schedule(guided)
#endif
    for(n=0; n<(int) maxn; n++)
    {
      void *temp=0;
      size_t oldsize=(ptrs[n] && (flags & M2_ZERO_MEMORY)) ? malloc_usable_size(ptrs[n]) : 0;
      if(ptrs[n]) /* resize */
      {
        if(!(flags & M2_PREVENT_MOVE))
          temp=alignment ? aligned_realloc(ptrs[n], alignment, _size) : realloc(ptrs[n], _size);
      }
      else /* allocate */
      {
        temp=alignment ? aligned_alloc(alignment, _size) : malloc(_size);
      }
      if(temp) /* success */
      {
        ptrs[n]=temp;
        if(errnos) errnos[n]=0;
        if((flags & M2_ZERO_MEMORY) && _size>oldsize)
          memset((void *)((char *) ptrs[n] + oldsize), 0, _size-oldsize);
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

_Bool batch_alloc2(int *errnos, struct mallocation2 **RESTRICT mdataptrs, size_t *RESTRICT count, size_t alignment, size_t reserve, uintmax_t flags)
{
  int n;
  size_t maxn=*count, _count=0;
#ifdef _OPENMP
#pragma omp parallel for reduction(+:_count) schedule(guided)
#endif
  for(n=0; n<(int) maxn; n++)
  {
    if(!mdataptrs[n] || !mdataptrs[n]->size) /* free */
    {
      if(mdataptrs[n] && mdataptrs[n]->ptr)
      {
#ifdef _MSC_VER
        msvcfree(mdataptrs[n]->ptr);
#else
        free(mdataptrs[n]->ptr);
#endif
        mdataptrs[n]->ptr=0;
      }
      _count++;
    }
    else
    {
      void *temp=0;
      size_t oldsize=(mdataptrs[n]->ptr && (flags & M2_ZERO_MEMORY)) ? malloc_usable_size(mdataptrs[n]->ptr) : 0;
      if(mdataptrs[n]->ptr) /* resize */
      {
        if(!(flags & M2_PREVENT_MOVE))
          temp=alignment ? aligned_realloc(mdataptrs[n]->ptr, alignment, mdataptrs[n]->size) : realloc(mdataptrs[n]->ptr, mdataptrs[n]->size);
      }
      else /* allocate */
      {
        temp=alignment ? aligned_alloc(alignment, mdataptrs[n]->size) : malloc(mdataptrs[n]->size);
      }
      if(temp) /* success */
      {
        mdataptrs[n]->ptr=temp;
        mdataptrs[n]->size=malloc_usable_size(temp);
        if(errnos) errnos[n]=0;
        if((flags & M2_ZERO_MEMORY) && mdataptrs[n]->size>oldsize)
          memset((void *)((char *) mdataptrs[n]->ptr + oldsize), 0, mdataptrs[n]->size-oldsize);
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

_Bool batch_alloc5(int *errnos, struct mallocation5 **RESTRICT mdataptrs, size_t *RESTRICT count)
{
  int n;
  size_t maxn=*count, _count=0;
#ifdef _OPENMP
#pragma omp parallel for reduction(+:_count) schedule(guided)
#endif
  for(n=0; n<(int) maxn; n++)
  {
    if(!mdataptrs[n] || !mdataptrs[n]->size) /* free */
    {
      if(mdataptrs[n] && mdataptrs[n]->ptr)
      {
#ifdef _MSC_VER
        msvcfree(mdataptrs[n]->ptr);
#else
        free(mdataptrs[n]->ptr);
#endif
        mdataptrs[n]->ptr=0;
      }
      _count++;
    }
    else
    {
      void *temp=0;
      size_t oldsize=(mdataptrs[n]->ptr && (mdataptrs[n]->flags & M2_ZERO_MEMORY)) ? malloc_usable_size(mdataptrs[n]->ptr) : 0;
      if(mdataptrs[n]->ptr) /* resize */
      {
        if(!(mdataptrs[n]->flags & M2_PREVENT_MOVE))
          temp=mdataptrs[n]->alignment ? aligned_realloc(mdataptrs[n]->ptr, mdataptrs[n]->alignment, mdataptrs[n]->size) : realloc(mdataptrs[n]->ptr, mdataptrs[n]->size);
      }
      else /* allocate */
      {
        temp=mdataptrs[n]->alignment ? aligned_alloc(mdataptrs[n]->alignment, mdataptrs[n]->size) : malloc(mdataptrs[n]->size);
      }
      if(temp) /* success */
      {
        mdataptrs[n]->ptr=temp;
        mdataptrs[n]->size=malloc_usable_size(temp);
        if(errnos) errnos[n]=0;
        if((mdataptrs[n]->flags & M2_ZERO_MEMORY) && mdataptrs[n]->size>oldsize)
          memset((void *)((char *) mdataptrs[n]->ptr + oldsize), 0, mdataptrs[n]->size-oldsize);
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

#if !defined(__linux__) /* Linux's glibc already provides malloc_usable_size */
size_t malloc_usable_size(void *ptr)
{
#ifdef _MSC_VER
  return _msize(ptr);
#elif defined(__FreeBSD__) || defined(__APPLE__)
  return malloc_size(ptr);
#endif
}
#endif

N1519MALLOCNOALIASATTR N1519MALLOCPTRATTR void *try_aligned_realloc(void *ptr, size_t alignment, size_t size)
{
#ifdef _MSC_VER
  return _expand(ptr, size);
#else
  /* Always fail */
  return 0;
#endif
}

N1519MALLOCNOALIASATTR N1519MALLOCPTRATTR void *try_realloc(void *ptr, size_t size)
{
#ifdef _MSC_VER
  return _expand(ptr, size);
#else
  /* Always fail */
  return 0;
#endif
}

void msvcfree(void *ptr)
{
#ifdef _MSC_VER
  errno=0;
#endif
  free(ptr);
#ifdef _MSC_VER
  if(errno)
    _aligned_free(ptr);
#endif
}

