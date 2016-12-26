CASHPACK - The C Anti-State HPACK library
=========================================

.. image:: logo.png
   :alt: cashpack logo

cashpack is a stateless event-driven HPACK codec aimed at embedded systems.
It is meant to work with HTTP/2 or similar protocols in the sense that some
assumptions made by the library would not work in all situations. For example,
HTTP/2 doesn't allow upper-case characters in header names, neither does
cashpack.

.. image:: https://travis-ci.org/Dridi/cashpack.svg
   :alt: Travis CI badge
   :target: https://travis-ci.org/Dridi/cashpack/
.. image:: https://scan.coverity.com/projects/7758/badge.svg
   :alt: Coverity Scan badge
   :target: https://scan.coverity.com/projects/dridi-cashpack/
.. image:: https://codecov.io/gh/Dridi/cashpack/branch/master/graph/badge.svg
   :alt: Codecov badge
   :target: https://codecov.io/gh/Dridi/cashpack

It started as a research project and became the basis for Varnish Cache's
HPACK implementation. To learn more about its origins, you can watch this
talk_ from the first edition of VarnishCon.

.. _talk: https://www.infoq.com/fr/presentations/varnishcon-dridi-boukelmoune-hpack-vs-varnish-cache

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

An example of the library usage can be found in the `cashpack(3)` manual.

Contributing
------------

The best way to contribute to cashpack is to use it on platforms other than
x86_64 GNU/Linux with glibc and report failures. Despite a paranoid coding
style and the benefits of open-source [1]_ there may be security issues.

Design goals
------------

0. Code clarity

This goal overrides all the others. Code clarity prevails over other goals
unless numbers show that optimizations are required. Because of the natural
indirection created by an event-driven approach (it leads to the fifth circle
of the callback hell) accidental complexity should be kept to a minimum.

1. Stateless event drivers

An HPACK implementation cannot be completely stateless, because a dynamic
table needs to be maintained. There is also the need to be able to decode
partial HPACK blocks incrementally. The event-driven API on the other hand is
completely stateless, and can carry user-defined state.

2. Single allocation

An HPACK instance requires a single allocation, and it is possible to plug
your own allocator. An update of the dynamic table size will require a
single reallocation too.

By default cashpack relies on malloc(3), realloc(3) and free(3).

3. Single-copy in the library code

Except when a field needs to be inserted in the dynamic table, cashpack may
copy, decode, or encode data exactly once. The goal used to be zero-copy but
proved to be harder to implement and less efficient. Insertions in the dynamic
tables copy data a second "single" time, but contents need to be moved prior
to that. Under certain circumstances, data may need to be moved in several
steps, but only during encoding. This is no longer a DOS vector for decoding.

Evictions from the dynamic table on the other hand are very cheap.

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

cashpack is not responsible for writing or reading HPACK blocks, it will only
send events during decoding or encoding.

6. No locking

Assuming an HTTP/2 or similar usage, no locking is required. The decoding
or encoding should happen in the HTTP/2 RX or TX loop, which is ordered.

7. Decoding as a state machine

Events are triggered following deterministic finite state machines, which
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
