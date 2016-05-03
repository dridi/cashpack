CASHPACK - The C Anti-State HPACK library
=========================================

cashpack is a stateless event-driven HPACK codec. It is meant to work with
HTTP/2 or similar protocols in the sense that some assumptions made by the
library would not work in all situations. For instance, it is not possible
to feed the decoder with partial HPACK contents.

.. image:: https://travis-ci.org/Dridi/cashpack.svg
.. image:: https://scan.coverity.com/projects/7758/badge.svg

How to use
-----------

cashpack relies on autotools for building, and a range of tools for testing
and code coverage. The basic usage is as follows::

   $ path/to/cashpack/bootstrap \
   >        [--with-memcheck]   \
   >        [--with-asan]       \
   >        [--with-msan]       \
   >        [--with-ubsan]      \
   >        [--with-lcov]
   $ make check

The first command will reveal the missing bits, and the second the potential
failures. Code coverage MUST be turned off when the test suite is used for
checking because it turns off assertions.

The ``bootstrap`` script needs to be run only once. In order to reconfigure
the build tree, you can use autoconf's ``configure`` script. Command-line
arguments to the ``bootstrap`` script are passed to ``configure``.

For code coverage, the simplest way to get a report is as follows::

   $ path/to/cashpack/bootsrap --with-lcov
   $ make lcov
   $ xdg-open lcov/index.html

An example of the library usage can be found in the test suite.

Contributing
------------

The best way to contribute to cashpack is to use it on platforms other than
x86_64 GNU/Linux with glibc and report failures. You shouldn't be surprised to
discover alignment issues on other architectures. Despite a semi-paranoid
coding style and the benefits of open-source [1]_ there may be security issues.

Design goals
------------

0. Code clarity

This goal overrides all the others. Code clarity prevails over other goals
unless numbers show that optimizations are required. Because of the natural
indirection created by an event-driven approach (it leads to the fifth circle
of the callback hell) accidental complexity should be kept to a minimum. There
is also almost no documentation other than this README.

1. Maintain no state besides the dynamic table

An HPACK implementation cannot be completely stateless, because a dynamic
table needs to be maintained. Relying on the assumption that HTTP/2 will
always decode complete HPACK sequences, statelessness is achieved using an
event-driven API.

2. Single allocation

An HPACK instance requires a single allocation, and it is possible to plug
your own allocator. An update of the dynamic table size will require a
single reallocation too.

By default cashpack relies on malloc(3), realloc(3) and free(3).

3. Zero-copy in the library code

Except when a field needs to be inserted in the dynamic table, cashpack does
not copy data around. However an insertion in the dynamic table requires to
move the existing contents to make room for the new field. Evictions on the
other hand are cheap.

4. Self-contained

Besides the standard C library, cashpack doesn't pull anything at run time.

It can be verified by looking at the shared object::

   $ make
   $ nm -D lib/.libs/libhpack.so |           # list dynamic symbols
   > awk 'NF == 2 && $1 == "U" {print $2}' | # keep undefined symbols
   > grep -v '^_'                            # drop __weak symbols
   free
   malloc
   memcpy
   memmove
   memset
   realloc
   strchr
   strlen

5. No system calls

Again on the assumption that decoded HPACK blocks are always complete,
there is no need to make system calls in the decoding code path for tasks
such as fetching data.

6. No locking

Assuming an HTTP/2 or similar usage, no locking is required. The decoding
or encoding should happen in the HTTP/2 RX or TX loop, which is ordered.

7. Decoding as a state machine

Events are triggered following a deterministic finite state machine, which
hopefully should help better understand the decoding flow.

8. Tight API

The HPACK state is opaque to the library user. It is however possible to
inspect the dynamic table in order to know its contents. This is done with
the decoder's event driver, but in a simpler state machine.

9. A human-friendly test suite

It is possible to just copy/paste hexdumps and other bits from the HPACK
specification in order to write tests. All examples from RFC 7541 are
already covered by the test suite.

There are no unit tests, instead C programs are written to interact with
the library with a Bourne Shell test suite on top of them.

10. Abuse 3-letters abbreviations and acronyms

Function names are actually made up using proper words, but the rest is a
collection of 3-letter symbols. 4-letter symbols are tolerated as long as
enough 2-letter symbols restore the balance.

.. [1] Having many eyes not reviewing the code
