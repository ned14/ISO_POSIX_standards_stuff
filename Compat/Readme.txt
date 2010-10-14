N1519 C99 compatible implementation
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
(C) 2010 Niall Douglas http://www.nedproductions.biz/

Thanks to various proprietary extensions on the supported platforms, this has a fairly complete implementation apart from address space reservation and constant time frees. It will make use of OpenMP if available and if your system allocator allows parallel entry then the performance of the batch operators is pretty good. Except when using aligned blocks on MSVC, you can interchange blocks returned by the n1519_* functions and the standard C99 malloc API.

Notes for Microsoft Windows:
-=-=-=-=-=-=-=-=-=-=-=-=-=-=
Note that due to MSVC's CRT free() implementation not being able to handle aligned blocks allocated via its own _aligned_* functions, you CANNOT interchange aligned blocks with non-aligned ones. The n1519_free() function knows how to handle both kinds of block, but n1519_realloc() does not.

Non-relocating resizes are supported thanks to a proprietary API.

Notes for POSIX:
-=-=-=-=-=-=-=-=
Note that even with proprietary APIs that aligned realloc has no support on Linux, BSD or Apple Mac OS X. As a result, this always aligned allocates a new block, memory copies and frees the old block.

Note that even with proprietary APIs M2_PREVENT_MOVE has no support on Linux, BSD or Apple Mac OS X. As a result any code which makes use of this flag (e.g. try_realloc(), try_aligned_realloc()) will always fail. Generally this isn't a problem, as most code tries non-relocating resizes and if those fail, they perform a full copy/move into a new allocation.
