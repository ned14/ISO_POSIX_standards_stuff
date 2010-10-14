N1519 C99 compatible implementation
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
(C) 2010 Niall Douglas http://www.nedproductions.biz/

Thanks to various proprietary extensions on the supported platforms, this has a fairly complete implementation. It will make use of OpenMP if available and if your system allocator allows parallel entry then the performance of the batch operators is pretty good.

Notes for Microsoft Windows:
-=-=-=-=-=-=-=-=-=-=-=-=-=-=
Note that due to MSVC's CRT free() implementation not being able to handle aligned blocks allocated via its own _aligned_* functions, a msvcfree() function is supplied which does handle this. You should do:

#define free(x) msvcfree(x)

... in any code using this library, though note that msvcfree() has poor performance when freeing aligned blocks.

Notes for POSIX:
-=-=-=-=-=-=-=-=
Note that even with proprietary APIs that aligned realloc has no support on Linux, BSD or Apple Mac OS X. As a result, this always aligned allocates a new block, memory copies and frees the old block.

Note that even with proprietary APIs M2_PREVENT_MOVE has no support on Linux, BSD or Apple Mac OS X. As a result any code which makes use of this flag (e.g. try_realloc(), try_aligned_realloc()) will always fail. Generally this isn't a problem, as most code tries non-relocating resizes and if those fail, they perform a full copy/move into a new allocation.
