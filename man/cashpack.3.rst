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

========
cashpack
========

------------------------------
the C Anti-State HPACK library
------------------------------

:Manual section: 3

SYNOPSIS
========

::

   #include <stdint.h>
   #include <stdlib.h>
   #include <hpack.h>

DESCRIPTION
===========

This is an overview of cashpack, a stateless event-driven HPACK codec written
in C. Instead of strictly following the HPACK specification, it implements
validation bits of HTTP/1.1 and HTTP/2 that don't require to maintain state.
HPACK is a stateful protocol by design, even though HTTP itself is stateless.
cashpack doesn't keep track of state beyond what's absolutely needed to comply
with HPACK, and it puts the user in a stateless event driver where the user
may manage more state.

MEMORY LAYOUT
=============

Regardless of the number of insertions done over the lifetime of an HPACK
instance, only a single allocation is needed. This makes cashpack a good fit
for embedded systems or in general strong requirements on memory management.
Updating the dynamic table size may however require a reallocation.

A single allocation means that the HPACK data structure and its dynamic table
are allocated at once, packed together. The dynamic table is literally managed
as a FIFO buffer. Consider a table with a size of 96 octets in which a header
``"name: value"`` is inserted::

    | 0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f |
    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    |                                                               |
    +-                30 octets of overhead                -+---+---+
    |                                                       | n   a
    +---+---+---+---+---+---+---+---+---+-------------------+---+---+
      m   e | ¶ | v   a   l   u   e | ¶ |                           |
    +---+---+---+---+---+---+---+---+---+                          -+
    |                                                               |
    +-                                                             -+
    |               55 octets of empty/unused space                 |
    +-                                                             -+
    |                                                               |
    +---------------------------------------------------------------+

The ¶ symbol represents the NUL character, meaning that strings in the dynamic
table are null-terminated and can be used with functions expecting a C string.
HPACK estimates an optimistic overhead of about 32 octets per entry, but with
cashpack this is exactly what you get: 30 octets for house-keeping plus 2 null
characters.

Now let's insert a new field ``"other: header"`` in the table::

    | 0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f |
    +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    |                                                               |
    +-                30 octets of overhead                -+---+---+
    |                                                       | o   t
    +---+---+---+---+---+---+---+---+---+-------------------+---+---+
      h   e   r | ¶ | h   e   a   d   e   r | ¶ |                   |
    +---+---+---+---+---+---+---+---+---+---+---+                  -+
    |                                                               |
    +-      30 octets of overhead       +---+---+---+---+---+---+---+
    |                                   | n   a   m   e | ¶ | v   a
    +---+---+---+---+-------------------+---+---+---+---+---+---+---+
      l   u   e | ¶ |        12 octets of empty/unused space        |
    +---+---+---+---+-----------------------------------------------+

Insertions are expensive, because new entries will push existing entries
further in the FIFO. However only entries that remain after the insertion are
moved. Entries are effectively evicted before insertions, and evictions are
very cheap. It is just a matter of decrementing the number of entries and the
table consumption. The dynamic table is systematically maintained as a
contiguous list of entries, therefore a cashpack decoder never suffers
fragmentation because no allocation occur during a header field insertion.
A cashpack encoder maintains its table identically, there's no difference to
the point that they both share the same ``struct hpack *`` type.

LOCKING
=======

There is absolutely no locking in cashpack, a ``struct hpack *`` is left
completely unguarded. The only thread-safe operation for this data structure
is its allocation if you use the default allocator.

You shouldn't need to lock an HPACK structure because HTTP's transport must be
ordered. In HTTP/2 streams are multiplexed but header frames (and for what
it's worth all frames) are decoded in order too.

RFC 2616 stated that you need a TCP-like protocol to transport HTTP, and that
you may use a different protocol as long as it provides TCP-like reliability:

|
| > HTTP communication usually takes place over TCP/IP connections. [...]
| > HTTP only presumes a reliable transport; any protocol that provides such
| > guarantees can be used; the mapping of the HTTP/1.1 request and response
| > structures onto the transport data units of the protocol in question is
| > outside the scope of this specification.

The QUIC protocol is an example of a reliable HTTP transport over unreliable
UDP.

However TCP establishes full-duplex communications that are completely
independent. That too is not a problem because an HTTP/2 session will require
two HPACK contexts: one for the requests and one for the responses.

DECODER VS ENCODER
==================

The only difference between a decoder and an encoder are the operations they
each perform. A decoder and an encoder sharing both ends of an HPACK session
logically maintain a single dynamic table in sync, of which both maintain a
local copy on their ends. They share the same state (HTTP is otherwise a
stateless protocol) and this reflects on the type system as both decoders and
encoders have the same ``struct hpack *`` type.

When performing an HTTP/2 session, you will need to allocate both an encoder
and a decoder, one of each being either for requests or responses. Both HPACK
decoding and encoding operations are event-driven. Codecs' dynamic tables can
also be probed, once again using an event driver.

EVENTS
======

The list of events is the same for all event drivers, some of them only use a
subset. An event handler has the following signature:

|
| **typedef void hpack_callback_f(enum hpack_event_e** *evt*\ **,**
| **\     const char** *\*buf*\ **, size_t** *size*\ **,** \
    **void** *\*priv*\ **);**

When not ``NULL``, The *buf* argument always point to non-persistent memory
that should only be considered valid (unless documented otherwise) until the
callback returns.

The type ``enum hpack_event_e`` may take the following values:

.. include:: hpe2rst.rst

When dealing with events, unknown values should be ignored. Future versions of
cashpack may introduce new events. The values for existing events shall never
be changed.

EXAMPLE
=======

The following example shows how to read HTTP/2 frames and decode HPACK blocks.
Only HEADERS frames are read, which means that SETTINGS frames resizing the
dynamic table would be ignored and may lead to decoding errors for perfectly
fine frames. On the other hand, malformed HTTP/2 frames (eg. duplicate headers
in a single block) may be decoded without problems.

The headers blocks are printed for each stream and at the end, the dynamic
table is printed too. This program is stateless and doesn't have side effects
so it doesn't use the private pointer in its decoding callback:

.. include:: cashdump.src
    :literal:

The HTTP/2 frames in this example are stored in a binary file in the same
directory:

.. include:: cashdump.hex
    :literal:

Assuming a standard installation of cashpack, the program can be compiled and
will produce the following output for the frames dumped above:

.. include:: cashdump.out
    :literal:

FILES
=====

These are subject to difference depending on local installation conventions.

*${includedir}/hpack.h*
        cashpack include file

*${libdir}/libhpack.a*
        cashpack static archive

*${libdir}/libhpack.la*
        cashpack libtool archive

*${libdir}/libhpack.so*
        cashpack shared object

*${libdir}/pkgconfig/cashpack.pc*
        pkg-config file for cashpack

SEE ALSO
========

**hpack_decode**\(3),
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
**hpack_trim**\(3),
**libtool**\(1),
**pkg-config**\(1)
