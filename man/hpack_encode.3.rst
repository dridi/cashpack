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
|    **hpack_callback_f**         *\*cb*\ **;**
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
function for the lifetime of the HTTP session.

TODO

ENCODING FLAGS
==============

The encoding process of a header list is driven by flags that explain how to
interpret a ``struct hpack_field`` instance. Some flags can be combined and
others are mutual exclusive. Combining flags that aren't documented as working
together results in undefined behavior:

.. include:: hpf2rst.rst

ENCODING STATE MACHINE
======================

TODO

::

    (start)     .----.
       |        v    |
       +----> EVICT -'
       |        |
       |        v
       +----> TABLE ---.
       |       | .---. |
       |       v v   | |
       +----> EVICT -' |
       |        |      |
       |        v      |
       +----> TABLE <--'
       |        |
       |        |      .---.
       |        v      v   |
       '----> FIELD    EVICT
               ^ |       ^
               | |       |
               | '--->---+
               |         | .--.
               |         v v  |
               +--<---- DATA -'
               |         |
               |         v
    (end) <----+--<--- INSERT

::

    (start)
       |
       +----> EVICT*
       |        |
       |        v
       |      TABLE
       |        |
       |        v
       |      EVICT*
       |        |
       |        v
       |      TABLE?
       |        |
       |        v
       '----> FIELD ---> EVICT*
                ^          ^
                |          |
                |          v
                |        DATA*
                |          |
                |          v
    (end) <-----+------ INSERT?

RETURN VALUE
============

TODO

ERRORS
======

TODO

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
