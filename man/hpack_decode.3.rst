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

----------------------------
Decode a chunk of HPACK data
----------------------------

:Manual section: 3

SYNOPSIS
========

| **#include <stdlib.h>**
| **#include <hpack.h>**
|
| **typedef void hpack_decoded_f(**
| **\     void** *\*priv*\ **,**
| **\     enum hpack_evt_e** *evt*\ **,**
| **\     const char** *\*buf*\ **, size_t** *size*\ **);**
|
| **enum hpack_res_e hpack_decode(**
| **\     struct hpack** *\*hpack*\ **,**
| **\     const void** *\*buf*\ **,** size_t** *size*\ **,**
| **\     hpack_decoded_f** *cb*, **void** *\*priv*\ **);**

DESCRIPTION
===========

TODO

SEE ALSO
========

**cashpack**\(3),
**hpack_decoder**\(3),
**hpack_encoder**\(3),
**hpack_free**\(3),
**hpack_foreach**\(3)