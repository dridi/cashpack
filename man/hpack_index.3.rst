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

====================================================================
hpack_static, hpack_dynamic, hpack_tables, hpack_entry, hpack_search
====================================================================

----------------------------------
probe the contents of HPACK tables
----------------------------------

:Title upper: HPACK_INDEX
:Manual section: 3

SYNOPSIS
========

| **#include <stdint.h>**
| **#include <stdlib.h>**
| **#include <unistd.h>**
| **#include <hpack.h>**
|
| **#define HPACK_STATIC   61**
| **#define HPACK_OVERHEAD 32**
|
| **enum hpack_result_e hpack_static(hpack_event_f** *cb*\ **,** \
    **void** *\*priv*\ **);**
|
| **enum hpack_result_e hpack_dynamic(struct hpack** *\*hpack*\ **,**
| **\     hpack_event_f** *cb*\ **, void** *\*priv*\ **);**
|
| **enum hpack_result_e hpack_tables(struct hpack** *\*hpack*\ **,**
| **\     hpack_event_f** *cb*\ **, void** *\*priv*\ **);**
|
| **enum hpack_result_e hpack_entry(struct hpack** *\*hpack*\ **,**
| **\     size_t** *idx*\ **, const char** *\*\*nam*\ **, const char** \
    *\*\*val*\ **)**
|
| **enum hpack_result_e hpack_search(struct hpack** *\*hpack*\ **,**
| **\     size_t** *\*idx*\ **, const char** *\*nam*\ **, const char** \
    *\*val*\ **)**

DESCRIPTION
===========

This is an overview of dynamic table probing with cashpack, a stateless
event-driven HPACK codec written in C. Both HPACK decoders and encoders share
a dynamic table in sync, it is possible outside of block processing to probe
the contents of the dynamic table. However, the dynamic table entries are not
persistent and MUST be considered stale after the decoding or encoding of an
HPACK block if ``EVICT`` or ``INDEX`` events occurred.

The ``hpack_static()`` and ``hpack_dynamic()`` functions walk respectively the
static table and the dynamic table of *hpack* and calls *cb* for all entries
events, passing a *priv* pointer that can be used to maintain state. The
``hpack_static()`` function ALWAYS emits the same sequence of events. The
``hpack_tables()`` function probes both the static and dynamic table for its
*hpack* argument.

The ``hpack_entry()`` function extracts an entry from both tables based on a
global index *idx*. When the function returns, *nam* and *val* respectively
point to the name and value of the indexed field.

The ``hpack_search()`` function searches for a best match for *nam* and *val*
in both the static and dynamic tables of *hpack*. It sets *idx* to the field's
index or zero if none was found. If a full match is not found, it may match
a field's name instead and therefore *val* is allowed to be ``NULL``.

The ``HPACK_STATIC`` and ``HPACK_OVERHEAD`` macros represent respectively the
number of entries in the static table and the per-entry overhead in dynamic
tables, as per the RFC.

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

The ``hpack_static()``, ``hpack_dynamic()``, ``hpack_tables()`` and
``hpack_entry()`` functions return ``HPACK_RES_OK``.  On error, these
functions returns one of the listed errors.

The ``hpack_search()`` function returns ``HPACK_RES_OK`` for a full match
and ``HPACK_RES_NAM`` if only a field name matched.

ERRORS
======

The ``hpack_static()`` function can fail with the following errors:

``HPACK_RES_ARG``: *cb* is ``NULL``.

The ``hpack_dynamic()`` and ``hpack_tables()`` functions can fail with the
following errors:

``HPACK_RES_ARG``: *hpack* doesn't point to a valid codec or *cb* is ``NULL``.

The ``hpack_entry()`` function can fail with the following errors:

``HPACK_RES_ARG``: *hpack*, *nam* or *val* is ``NULL``.

``HPACK_RES_IDX``: *idx* is not a valid index.

The ``hpack_search()`` function can fail with the following errors:

``HPACK_RES_ARG``: *hpack* doesn't point to a valid codec or *nam* is
``NULL``.

``HPACK_RES_IDX``: *idx* no match found in the tables.

SEE ALSO
========

**cashpack**\(3),
**hpack_decode**\(3),
**hpack_decode_fields**\(3),
**hpack_decoder**\(3),
**hpack_dump**\(3),
**hpack_encode**\(3),
**hpack_encoder**\(3),
**hpack_event_id**\(3),
**hpack_free**\(3),
**hpack_limit**\(3),
**hpack_resize**\(3),
**hpack_skip**\(3),
**hpack_strerror**\(3),
**hpack_tables**\(3),
**hpack_trim**\(3)
