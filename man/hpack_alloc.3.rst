.. Copyright (c) 2016 Dridi Boukelmoune
.. All rights reserved.
..
.. Redistribution and use in source and binary forms, with or without
.. modification, are permitted provided that the following conditions
.. are met:
.. 1. Redistributions of source code must retain the above copyright
..    notice, this list of conditions and the following disclaimer.
.. 2. Redistributions in binary form must reproduce the above copyright
..    notice, this list of conditions and the following disclaimer in the
..    documentation and/or other materials provided with the distribution.
..
.. THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
.. ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
.. IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
.. ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
.. FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
.. DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
.. OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
.. HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
.. LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
.. OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
.. SUCH DAMAGE.

===========
hpack_alloc
===========

--------------------------------------
Allocate, resize and free HPACK tables
--------------------------------------

:Manual section: 3

SYNOPSIS
========

| **#include <stdlib.h>**
| **#include <hpack.h>**
|
| **typedef void \* hpack_malloc_f(size_t** *size*\ **);**
| **typedef void \* hpack_realloc_f(void** *\*ptr*\ **, size_t** *size*\ **);**
| **typedef void   hpack_free_f(void** *\*ptr*\ **);**
|
| **struct hpack_alloc {**
|   **hpack_malloc_f**  *\*malloc*\ **;**
|   **hpack_realloc_f** *\*realloc*\ **;**
|   **hpack_free_f**    *\*free*\ **;**
| **};**
|
| **extern const struct hpack_alloc *hpack_default_alloc;**
|
| **struct hpack * hpack_encoder(size_t** *max*\ **, \
    const struct hpack_alloc** *\*alloc*\ **);**
| **struct hpack * hpack_decoder(size_t** *max*\ **, \
    const struct hpack_alloc** *\*alloc*\ **);**
| **void hpack_free(struct hpack** *\**hpack*\ **);**

DESCRIPTION
===========

TODO

SEE ALSO
========

**cashpack**\(3),
**hpack_decode**\(3),
**hpack_foreach**\(3)
