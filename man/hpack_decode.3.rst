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

============
hpack_decode
============

---------------------
decode an HPACK block
---------------------

:Manual section: 3

SYNOPSIS
========

| **#include <stdint.h>**
| **#include <stdlib.h>**
| **#include <unistd.h>**
| **#include <hpack.h>**
|
| **struct hpack_decoding {**
|    **const void**       *\*blk*\ **;**
|    **size_t**           *blk_len*\ **;**
|    **void**             *\*buf*\ **;**
|    **size_t**           *buf_len*\ **;**
|    **hpack_callback_f** *\*cb*\ **;**
|    **void**             *\*priv*\ **;**
| **};**
|
| **enum hpack_result_e hpack_decode(struct hpack** *\*hpack*\ **,**
| **\     const struct hpack_decoding** *\*dec*\ **, unsigned** *cut*\ **);**

DESCRIPTION
===========

This is an overview of decoding with cashpack, a stateless event-driven HPACK
codec written in C. HPACK is a stateful protocol, but cashpack implements
decoding using a stateless event driver. The *hpack* parameter keeps track of
the decoding state, including the dynamic table updates, across multiple calls
of the ``hpack_decode()`` function for the lifetime of the HTTP session. The
*dec* parameter contains the decoding context for the block or partial block
being decoded.

The *blk* field is a pointer to the block or partial block memory of *blk_len*
octets. All block input octets are always consumed before requiring new input,
allowing the caller to reuse its buffer.

Decoding is performed in a working memory buffer *buf* of *buf_len* octets.
The buffer needs to be large enough to contain a fully decoded header list.
While this may not (rightly) be much stateless in your book, this single copy
in the buffer enables simpler internals for the library and a simpler usage.

At the end of a block, all headers names and values are written consecutively
as null-terminated character strings. It also means that NAME and VALUE yield
persistent strings in a message's lifetime. When an HPACK block is decoded in
several passes, the exact same *buf* and *buf_len* must be passed to all calls
to ``hpack_decode()``.

The *priv* pointer is passed to the *cb* callback for all the events.

If *cut* is zero, the HPACK block being decoded is expected to end with the
*dec->blk_len* octets.

DECODING STATE MACHINE
======================

The decoding process produces events matching the following state machine. The
detailed state machine describes the possible decoding events of a complete
HPACK block. If the block is decoded in more than one pass, the event driver
resumes where it left, strictly adhering to the possible transitions.

The state machine is laid out so that moving down means making progress in the
decoding process, and moving right implies a hierarchical relationship between
events. For instance a NAME event belongs after a FIELD event::

    (start)
       |
       +----> EVICT
       |        |
       |        v
       +----> TABLE ---.
       |        |      |
       |        v      |
       +----> EVICT    |
       |        |      |
       |        v      |
       +----> TABLE <--'
       |        |
       |        v
       '----> FIELD ---> NEVER ---.
               ^ |        |       |
               | |        v       |
               | +-----> EVICT    |
               | |       |  .-----'
               | |       |  |
               | |       v  v
               | '-----> NAME
               |           |
               |           v
               |         VALUE
               |           |
    (end) <----+-----<-----+
               ^           |
               |           v
               '-------- INDEX

If you are familiar with regular expressions, here is a translation of the
decoding state machine to a regular expressions using the initials of the
events names. ``NEVER`` is translated to lowercase ``n`` and ``NAME`` to
uppercase ``N``::

    ^(E?T(E?T)?)?(Fn?E?NVI?)+$

The state machine may look complex, but this is mainly due to dynamic table
events that *might* be emitted on many occasions. Here is the same state
machine, but ``FOO?`` ``BAR*`` and ``BAZ+`` events regex-like notations mean
that event ``FOO`` can happen zero to one time, ``BAR`` can happen zero to
many times and ``BAZ`` events can happen one to many times::

    (start)
       |
       +----> EVICT?
       |        |
       |        v
       |      TABLE
       |        |
       |        v
       |      EVICT?
       |        |
       |        v
       |      TABLE?
       |        |
       |        v
       '----> FIELD ---> NEVER?
                ^          |
                |          v
                |        EVICT?
                |          |
                |          v
                |        NAME
                |          |
                |          v
                |        VALUE
                |          |
                |          v
    (end) <-----+---<--- INDEX?

But the role of the dynamic table events is not directly related to the HTTP
message that is being decoded. If you focus on the events that help you build
a header list, it becomes a lot simpler::

    (start)
       |
       '---> FIELD ---> NAME
               ^         |
               |         v
               |       VALUE
               |         |
    (end) <----+-----<---'

This last state machine describes the events where ordering is key. If you
follow arrows in the detailed state machines, you will find that a ``NEVER``
event may be followed by an ``INDEX`` event. That is never the case, but in
order to keep the detailed state machines *simpler* this detail is omitted.

RETURN VALUE
============

The ``hpack_decode()`` function returns ``HPACK_RES_OK`` if *cut* is zero,
otherwise ``HPACK_RES_BLK``. On error, this function returns one of the listed
errors and makes the *hpack* argument improper for further use.

ERRORS
======

The ``hpack_decode()`` function can fail with the following errors:

``HPACK_RES_ARG``: *hpack* doesn't point to a valid decoder or *dec* contains
``NULL`` pointers or zero lengths, except *dec->priv* which is optional.

All other errors except ``HPACK_RES_BSY``, see ``hpack_strerror``\ (3) for the
details of all possible errors.

SEE ALSO
========

**cashpack**\(3),
**hpack_decoder**\(3),
**hpack_dynamic**\(3),
**hpack_encode**\(3),
**hpack_encoder**\(3),
**hpack_foreach**\(3),
**hpack_free**\(3),
**hpack_limit**\(3),
**hpack_resize**\(3),
**hpack_static**\(3),
**hpack_strerror**\(3),
**hpack_tables**\(3),
**hpack_trim**\(3)
