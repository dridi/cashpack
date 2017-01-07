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

===============================
hpack_encode, hpack_clean_field
===============================

---------------------
encode an HPACK block
---------------------

:Title upper: HPACK_ENCODE
:Manual section: 3

SYNOPSIS
========

| **#include <stdint.h>**
| **#include <stdlib.h>**
| **#include <unistd.h>**
| **#include <hpack.h>**
|
| **enum hpack_flag_e;**
|
| **struct hpack_field {**
|     **uint32_t** *flg*\ **;**
|     **uint16_t** *idx*\ **;**
|     **uint16_t** *nam_idx*\ **;**
|     **char**     *\*nam*\ **;**
|     **char**     *\*val*\ **;**
| **};**
|
| **struct hpack_encoding {**
|    **const struct hpack_field** *\*fld*\ **;**
|    **size_t**                   *fld_cnt*\ **;**
|    **void**                     *\*buf*\ **;**
|    **size_t**                   *buf_len*\ **;**
|    **hpack_event_f**            *\*cb*\ **;**
|    **void**                     *\*priv*\ **;**
| **};**
|
| **enum hpack_result_e hpack_encode(struct hpack** *\*hpack*\ **,**
| **\     const struct hpack_encoding** *\*dec*\ **, unsigned** *cut*\ **);**
|
| **enum hpack_result_e hpack_clean_field(struct hpack_field** \
    *\*field*\ **);**

DESCRIPTION
===========

This is an overview of encoding with cashpack, a stateless event-driven HPACK
codec written in C. HPACK is a stateful protocol, but cashpack implements
encoding using a stateless event driver. The *hpack* parameter keeps track of
the dynamic table updates across multiple calls of the ``hpack_encode()``
function for the lifetime of the HTTP session. The *enc* parameter contains
the encoding context for the block or partial block being encoded.

The *fld* field is a pointer to an array of *fld_cnt* HPACK fields. All fields
are always encoded before requiring new input, allowing the caller to reuse
its array. Encoding is performed in a working memory buffer *buf* of *buf_len*
octets, the working buffer can safely be reused in subsequent calls. All the
events are described in the ``cashpack``\ (3) manual. The *priv* pointer is
passed to the *cb* callback for all the events.

If *cut* is zero, the HPACK block being encoded is expected to end with the
*enc->fld_cnt* fields.

ENCODING FLAGS
==============

A header list is a set of key/values referenced by the fields *nam* and *val*
in ``struct hpack_field``. Those values may be omitted depending on the flags
set in the *flg* field. Indexed fields rely on the *idx* field instead, and
literal fields with an indexed name rely on *nam_idx*.

The encoding process of a header list is thus driven by flags that explain how
to interpret a ``struct hpack_field`` instance. Some flags can be combined and
others are mutual exclusive. Combining flags that aren't documented as working
together results in undefined behavior:

.. include:: hpf2rst.rst

ENCODING STATE MACHINE
======================

The encoding process produces events matching the following state machine. The
detailed state machine describes the possible encoding events of a complete
HPACK block. If the block is encoded in more than one pass, the event driver
resumes where it left, strictly adhering to the possible transitions. It can
however only resume at a FIELD event.

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
       |        |
       |        v
       '----> FIELD ---> EVICT
               ^ |         |
               | '---->----+
               |           | .--.
               |           v v  |
               +----<---- DATA -'
               |           |
               |           v
    (end) <----+----<--- INDEX

If you are familiar with regular expressions, here is a translation of the
encoding state machine to a regular expressions using the initials of the
events names::

    ^(E?T(E?T)?)?(FE?D+I?)+$

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
       '----> FIELD ---> EVICT?
                ^          |
                |          v
                |        DATA+
                |          |
                |          v
    (end) <-----+---<--- INDEX?

But the role of the dynamic table events is not directly related to the HTTP
message that is being decoded. If you focus on the events that help you build
an HPACK block, it becomes a lot simpler::

    (start)
       |
       '----> FIELD ---> DATA+
                ^         |
                |         |
    (end) <-----+----<----'

The FIELD event is merely here to allow changes before a header gets encoded.
You can for instance use table events to detect that fields with indexed flags
no longer reference valid indices, and turn them into field literals that may
or may not be inserted in the table. It is the mechanism that empowers caching
policies.

CLEANUP
=======

Once encoding is done, fields can be cleaned using the ``hpack_clean_field()``
function. It is equivalent to zeroing the *fld* parameter, except that checks
are performed and only fields relevant to the flags are cleaned. It is a means
for double-checking proper usage of the library. The *nam* and *val* pointers
may be set to ``NULL``, but if they point to memory that needs to be freed, it
MUST be done prior to calling ``hpack_clean_field()``.

RETURN VALUE
============

The ``hpack_encode()`` function returns ``HPACK_RES_OK`` if *cut* is zero,
otherwise ``HPACK_RES_BLK``. On error, this function returns one of the listed
errors and makes the *hpack* argument improper for further use.

The ``hpack_clean_field()`` function returns ``HPACK_RES_OK`` if the field's
structure was properly zeroed, otherwise ``HPACK_RES_ARG``.

ERRORS
======

The ``hpack_encode()`` function can fail with the following errors:

``HPACK_RES_ARG``: *hpack* doesn't point to a valid decoder or *dec* contains
``NULL`` pointers or zero lengths, except *dec->priv* which is optional.

All other errors except ``HPACK_RES_BSY``, see ``hpack_strerror``\ (3) for the
details of all possible errors.

EXAMPLE
=======

The following example shows how to keep track of insertions and evictions in
order to maintain proper indexed fields, in case they become stale as encoding
makes progress. The strategy used in this example is far from optimal, fields
are either taken from the index, or inserted in the dynamic table.

Header lists are read from the standard input, separated by blank lines. The
encoded header blocks are written on the standard output, without anything to
indicate the boundaries between blocks. Information about the encoding process
is logged to the standard error.

.. include:: cashdumb.src
    :literal:

In this example the header lists will be read from a file:

.. include:: cashdumb.txt
    :literal:

Assuming a standard installation of cashpack, the program can be compiled and
will produce the following logs, the binary output being ignored:

.. include:: cashdumb.out
    :literal:

SEE ALSO
========

**cashpack**\(3),
**hpack_decode**\(3),
**hpack_decoder**\(3),
**hpack_dynamic**\(3),
**hpack_encoder**\(3),
**hpack_foreach**\(3),
**hpack_free**\(3),
**hpack_limit**\(3),
**hpack_resize**\(3),
**hpack_static**\(3),
**hpack_strerror**\(3),
**hpack_tables**\(3),
**hpack_trim**\(3)
