pthreads Permit Object
-=-=-=-=-=-=-=-=-=-=-=
(C) 2010-2012 Niall Douglas http://www.nedproductions.biz/

Herein is a reference implementation for a proposed safe permit object for POSIX
threads. Its development came out of work done internally to ISO WG14 when making
the C11 release of the C programming language, but it was decided to submit the
proposal to the Austin Working Group for inclusion into POSIX after the 2012 TC1.

This reference implementation has been tested on:
 * Microsoft Windows 7 with Visual Studio 2010
 * Clang v3.1
 * GCC v4.6

Build notes:
-=-=-=-=-=-=
There is an almighty hack of C11 atomics and threading support for Windows in
../c11_compat.h. It is semantically correct however, but implements just enough
to get the reference object working.

On POSIX, C11's stdatomic.h and threads.h are not typically available as yet. As a
result, a copy of each culled from GCC v4.8's repo HEAD is present and used.
