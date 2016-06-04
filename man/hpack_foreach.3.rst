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

=========================================
hpack_static, hpack_dynamic, hpack_tables
=========================================

----------------------------------
probe the contents of HPACK tables
----------------------------------

:Title upper: hpack_foreach
:Manual section: 3

SYNOPSIS
========

| **#include <stdint.h>**
| **#include <stdlib.h>**
| **#include <hpack.h>**
|
| **typedef void hpack_decoded_f(**
| **\     void** *\*priv*\ **,**
| **\     enum hpack_event_e** *evt*\ **,**
| **\     const char** *\*buf*\ **, size_t** *size*\ **);**
|
| **enum hpack_result_e hpack_static(hpack_decoded_f** *cb*\ **,** \
    **void** *\*priv*\ **);**
|
| **enum hpack_result_e hpack_dynamic(struct hpack** *\*hpack*\ **,**
| **\     hpack_decoded_f** *cb*\ **, void** *\*priv*\ **);**
|
| **enum hpack_result_e hpack_tables(struct hpack** *\*hpack*\ **,**
| **\     hpack_decoded_f** *cb*\ **, void** *\*priv*\ **);**

DESCRIPTION
===========

This is an overview of dynamic table probing with cashpack, a stateless
event-driven HPACK codec written in C. Both HPACK decoders and encoders share
a dynamic table in sync, it is possible outside of block processing to probe
the contents of the dynamic table. However, the dynamic table entries are not
persistent and MUST be considered stale after the decoding or encoding of an
HPACK block if ``EVICT`` or ``INSERT`` events occurred.

The ``hpack_static()`` and ``hpack_dynamic()`` functions walk respectively the
static table and the dynamic table of *hpack* and calls *cb* for all entries
events, passing a *priv* pointer that can be used to maintain state. The
``hpack_static()`` function ALWAYS emits the same sequence of events. The
``hpack_tables()`` function probes both the static and dynamic table for its
*hpack* argument.

FOREACH STATE MACHINE
=====================

The state machine is very straightforward. For each field, exactly three
events are emitted in this order: ``FIELD``, ``NAME`` and ``VALUE``.

::

    (start)
       |
       +---> FIELD ---> NAME
       |       ^          |
       v       |          v
     (end) <---+------- VALUE

The ``NAME`` and ``VALUE`` events provide null-terminated character strings
pointing directly inside the dynamic table. Dereferencing stale pointers is
undefined behavior. See ``cashpack``\ (3) for more details on the events
semantics. Static entries are always safe to dereference.

RETURN VALUE
============

The ``hpack_static()`` ``hpack_dynamic()`` and  ``hpack_tables()`` functions
return ``HPACK_RES_OK``. On error, these functions returns one of the listed
errors.

ERRORS
======

The ``hpack_static()`` function can fail with the following errors:

``HPACK_RES_ARG``: *cb* is ``NULL``.

The ``hpack_dynamic()`` and ``hpack_tables()`` functions can fail with the
following errors:

``HPACK_RES_ARG``: *hpack* doesn't point to a valid codec or *cb* is ``NULL``.

``HPACK_RES_BSY``: the codec is busy processing an HPACK block.

SEE ALSO
========

**cashpack**\(3),
**hpack_decoder**\(3),
**hpack_encoder**\(3),
**hpack_free**\(3),
**hpack_resize**\(3),
**hpack_trim**\(3),
**hpack_decode**\(3),
**hpack_encode**\(3),
**hpack_foreach**\(3),
**hpack_dynamic**\(3),
**hpack_tables**\(3),
**hpack_static**\(3),
**hpack_strerror**\(3)
