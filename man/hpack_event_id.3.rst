.. License: BSD-2-Clause
.. (c) 2020 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>

==============
hpack_event_id
==============

---------------------------
turn an event into a string
---------------------------

:Manual section: 3

SYNOPSIS
========

| **#include <sys/types.h>**
| **#include <stdint.h>**
| **#include <hpack.h>**
|
| **enum hpack_event_e;**
|
| **const char * hpack_event_id(enum hpack_event_e** *evt*\ **);**

DESCRIPTION
===========

This is an overview of event functions in cashpack, a stateless event-driven
HPACK codec written in C.

The ``hpack_event_id()`` function is a thread-safe function that translates an
event code into a human readable string identifier.

RETURN VALUE
============

The ``hpack_event_id()`` function returns a constant string corresponding to
an event enumerator, or ``NULL`` for unknown values.

SEE ALSO
========

**cashpack**\(3),
**hpack_decode**\(3),
**hpack_decode_fields**\(3),
**hpack_decoder**\(3),
**hpack_dump**\(3),
**hpack_dynamic**\(3),
**hpack_encode**\(3),
**hpack_encoder**\(3),
**hpack_entry**\(3),
**hpack_free**\(3),
**hpack_limit**\(3),
**hpack_monitor**\(3),
**hpack_resize**\(3),
**hpack_search**\(3),
**hpack_skip**\(3),
**hpack_static**\(3),
**hpack_strerror**\(3),
**hpack_tables**\(3),
**hpack_trim**\(3)
