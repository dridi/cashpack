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

==============
hpack_strerror
==============

------------------------------------------
return a string describing an error number
------------------------------------------

:Manual section: 3

SYNOPSIS
========

| **#include <stdint.h>**
| **#include <stdlib.h>**
| **#include <hpack.h>**
|
| **enum hpack_result_e;**
|
| **const char * hpack_strerror(enum hpack_result_e** *res*\ **);**

DESCRIPTION
===========

This is an overview of error handling in cashpack, a stateless event-driven
HPACK codec written in C. cashpack offers a detailed set of result codes for
error handling. Most of the result codes map to errors, and most errors leave
a defunct codec that can no longer be used.

The ``hpack_strerror()`` function is a thread-safe function that translates an
error code into a human readable short description.

RESULT CODES
============

Only errors have a negative result code, so functions can be called with a
generic style::

    retval = hpack_xxx(...);
    if (retval < 0) {
    	/* error handling */
    }

The list of possible results:

.. include:: hpr2rst.rst

RETURN VALUE
============

The ``hpack_strerror()`` function returns a constant string corresponding to
one of the short descriptions detailed above, or ``NULL`` for unknown values.

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
**hpack_foreach**\(3)
**hpack_dynamic**\(3),
**hpack_tables**\(3),
**hpack_static**\(3),
