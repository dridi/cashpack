.. Copyright (c) 2016-2020 Dridi Boukelmoune
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

===============================================================================
hpack_decoder, hpack_encoder, hpack_free, hpack_resize, hpack_limit, hpack_trim
===============================================================================

--------------------------------------
allocate, resize and free HPACK codecs
--------------------------------------

:Title upper: HPACK_ALLOC
:Manual section: 3

SYNOPSIS
========

| **#include <sys/types.h>**
| **#include <stdint.h>**
| **#include <hpack.h>**
|
| **typedef void \* hpack_malloc_f(size_t** *size*\ **, void** *\*priv*\ **);**
| **typedef void \* hpack_realloc_f(void** *\*ptr*\ **, size_t** *size*\ **, \
    void** *\*priv*\ **);**
| **typedef void   hpack_free_f(void** *\*ptr*\ **, void** *\*priv*\ **);**
|
| **struct hpack_alloc {**
|     **hpack_malloc_f**  *\*malloc*\ **;**
|     **hpack_realloc_f** *\*realloc*\ **;**
|     **hpack_free_f**    *\*free*\ **;**
|     **void**            *\*priv*\ **;**
| **};**
|
| **extern const struct hpack_alloc** *\*hpack_default_alloc;*
|
| **struct hpack * hpack_decoder(size_t** *max*\ **, ssize_t** *mem*\ **,**
| **\     const struct hpack_alloc** *\*alloc*\ **);**
| **struct hpack * hpack_encoder(size_t** *max*\ **, ssize_t** *mem*\ **,**
| **\     const struct hpack_alloc** *\*alloc*\ **);**
| **void hpack_free(struct hpack** *\**hpackp*\ **);**
|
| **enum hpack_result_e hpack_resize(struct hpack** *\*\*hpackp*\ **,** \
    **size_t** *max*\ **);**
| **enum hpack_result_e hpack_limit(struct hpack** *\*\*hpackp*\ **,** \
    **size_t** *max*\ **);**
| **enum hpack_result_e hpack_trim(struct hpack** *\*\*hpackp*\ **);**

DESCRIPTION
===========

This is an overview of memory management in cashpack, a stateless event-driven
HPACK codec written in C. cashpack offers a great deal of control over memory
management and follows a single-allocation principle. Under very specific
circumstances, the dynamic table is effectively resized and that may trigger a
reallocation. It is possible to allocate static or pseudo-static memory from
some sort of memory pool or even on the stack.

MEMORY MANAGEMENT
=================

Memory management is composed of three very familiar operations: ``malloc()``,
``realloc()`` and ``free()``. These operations may rely on external state that
may be passed using the *\*priv* pointer in ``struct hpack_alloc``. A private
pointer's destination MUST outlive HPACK codecs created using it.

The memory management functions, if specified, MUST follow the same semantics
as **malloc**\(3) **realloc**\(3) and **free**\(3). Especially, the ``malloc``
operation is responsible for meeting platform-specific alignment requirements.
It's NOT necessary to initialize memory like **calloc**\(3) would do, cashpack
will always write to memory before reading it. *hpack_default_alloc* is here
to provide a thin wrapper on top of the libc standard functions.

cashpack doesn't get in the way of memory management and doesn't prevent doing
things like defragmentation. It is perfectly safe to move the data structure,
it is both persistent and self-contained. The only references outside of the
data structure are the pointers for the memory manager's functions and state,
they need to be persistent too and can't be changed once allocated.

One way to achieve single-allocation despite a resize of the table is to
allocate the desired eventual size with the ``malloc()`` operation and turn
the ``realloc()`` one into a no-op.

ALLOCATION
==========

The ``hpack_decoder()`` and ``hpack_encoder()`` functions create respectively
HPACK decoders and encoders. Both  share the same ``struct hpack *`` type for
common operations, but using a decoder for an encoding operation will fail and
vice versa. The *max* argument defines the dynamic table initial maximum size.
For instance, the initial size for HTTP/2 is 4096 octets. The *alloc* argument
points to the memory manager that performs actual memory allocations.

The absolute maximum size for the dynamic table is 65535 octets.

The *mem* argument allows you to define the initial allocation size for the
dynamic table. This is the safest way to guarantee a single allocation. A
decoder that plans to resize the dynamic table must defer the actual resize
operation, in HTTP/2 it happens after a settings acknowledgement. An encoder
on the other hand must obey the size changes mandated by its peer decoder, so
setting *mem* to a non-negative value is the single-allocation equivalent to::

    /* omitting error handling */
    hp = hpack_encoder(max, -1, hpack_default_alloc);
    hpack_limit(&hp, mem);

The ``hpack_free()`` function frees the space allocated to HPACK codecs. The
memory manager may not provide a free operation, but it may still be useful to
properly dispose of a codec. The function will wipe the pointer and make the
data structure unusable to reduce risks of double-frees or uses-after-free.

RESIZING
========

The ``hpack_resize()`` function allows the resize *\*hpackp*'s dynamic table
outside of the HPACK protocol. With HTTP/2, that is be upon the acknowledgment
of a SETTINGS frame. If the new maximum size *max* is greater than available
memory for the dynamic table, a reallocation is performed. If *max* is lower
than available memory, the allocation is left untouched.

After out-of-band resizes, the HPACK protocol expects up to two table updates
in the following HPACK block. An HPACK decoder checks the expected updates
when the next block is decoded with the ``hpack_decode()`` function. An HPACK
encoder automatically includes table updates in the next block encoded with
the ``hpack_encode()`` function.

The ``hpack_limit()`` function allows an encoder to limit to *max* the size of
the dynamic table for *hpackp* below the maximum decided by the peer decoder.
The limit is then sent as a table update when the next header list is encoded,
and overrides any subsequent calls to ``hpack_resize()``. Once applied, the
limit doesn't need to be reapplied every time the decoder decides to change
the maximum.

The ``hpack_trim()`` function performs a reallocation if the available memory
for the dynamic table is greater than its maximum size. This reallocation may
fail without consequences on the HPACK codec.

RETURN VALUE
============

The ``hpack_decoder()`` and ``hpack_encoder()`` functions return a pointer to
the allocated codec. On error, these functions return NULL. Errors include
invalid parameters or a failed allocation.

The ``hpack_resize()`` ``hpack_limit()`` and ``hpack_trim()`` functions return
``HPACK_RES_OK``. On error, these functions may return various errors and
``hpack_resize()`` may make its *hpackp* argument improper for further use.

ERRORS
======

The ``hpack_resize()`` ``hpack_limit()`` and ``hpack_trim()`` functions can
fail with the following errors:

``HPACK_RES_ARG``: *hpackp*/*hpack* is ``NULL`` or points to a ``NULL`` or
defunct codec.

``HPACK_RES_BSY``: the codec is busy processing an HPACK block.

``HPACK_RES_LEN``: the new size exceeds 65535 or the memory manager has no
``realloc`` operation to grow the table.

``HPACK_RES_OOM``: the reallocation failed.

SEE ALSO
========

**cashpack**\(3),
**hpack_decode**\(3),
**hpack_decode_fields**\(3),
**hpack_dump**\(3),
**hpack_dynamic**\(3),
**hpack_encode**\(3),
**hpack_entry**\(3),
**hpack_event_id**\(3),
**hpack_search**\(3),
**hpack_skip**\(3),
**hpack_static**\(3),
**hpack_strerror**\(3),
**hpack_tables**\(3),
**malloc**\(3),
**realloc**\(3),
**free**\(3)
