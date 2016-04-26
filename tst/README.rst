The cashpack test suite
=======================

General structure
-----------------

The test suite is written in Bourne Shell in order to improve readability in
ways that can hardly be achieved in C. A lot of plumbing is involved at two
levels:

- a ``common.sh`` file that contains all the logic
- backed by C programs that use the library

The common shell rules give some degree of isolation to the test cases by
setting up a temp working directory. It also abstracts most of the setup so
that the test cases contain as little boilerplate as possible.

The C programs serve as bindings between the C library and the shell scripts,
offering a convenient interface even for edge cases. Reaching a particular
line or branch from outside the library should always be straightforward given
the relative simplicity of HPACK.

The typical structure of a test case is the following::

    . "$(dirname "$0")"/common.sh

    mk_hex <<EOF
    [...]
    EOF

    mk_msg <<EOF
    [...]
    EOF

    mk_tbl <<EOF
    [...]
    EOF

    mk_enc <<EOF
    [...]
    EOF

    tst_decode
    tst_encode

All the ``mk_*`` functions create specific files from the *heredocs* they are
fed, possibly while applying transformations. What helps making a test case
readable is the input of those functions that are kept as close as possible to
the specification:

- ``mk_hex``: creates a binary file from a *hexdump* that uses the exact same
  format as RFC 7541 that defines HPACK. For example the *hexdump* used in the
  appendix C.5.1. can be cut and pasted verbatim from the RFC::

      mk_hex <<EOF
      4803 3330 3258 0770 7269 7661 7465 611d | H.302X.privatea.
      4d6f 6e2c 2032 3120 4f63 7420 3230 3133 | Mon, 21 Oct 2013
      2032 303a 3133 3a32 3120 474d 546e 1768 |  20:13:21 GMTn.h
      7474 7073 3a2f 2f77 7777 2e65 7861 6d70 | ttps://www.examp
      6c65 2e63 6f6d                          | le.com
      EOF

- ``mk_msg``: the HTTP message resulting from or leading to the *hexdump*. It
  can again be cut and pasted from the RFC::

      mk_msg <<EOF
      :status: 302
      cache-control: private
      date: Mon, 21 Oct 2013 20:13:21 GMT
      location: https://www.example.com
      EOF

- ``mk_tbl``: the contents of the dynamic table after decoding or encoding of
  an HTTP message using HPACK. One more time it's taken verbatim from the
  RFC::

      mk_tbl <<EOF
      [  1] (s =  63) location: https://www.example.com
      [  2] (s =  65) date: Mon, 21 Oct 2013 20:13:21 GMT
      [  3] (s =  52) cache-control: private
      [  4] (s =  42) :status: 302
            Table size: 222
      EOF

- ``mk_enc``: this function actually uses its own DSL for representing the
  encoding process of an HTTP message. The encoding/decoding process is very
  detailed in the RFC, so parsing it would most likely increase complexity and
  decrease readability. Instead, the encoding for appendix C.5.1. looks like
  this::

      mk_enc <<EOF
      dynamic idx 8 str 302
      dynamic idx 24 str private
      dynamic idx 33 str Mon, 21 Oct 2013 20:13:21 GMT
      dynamic idx 46 str https://www.example.com
      EOF


It is also possible to increase the readability by allowing empty lines and
comments, lines starting with a ``#`` symbol. This is done transparently by
the shell functions and comes handy when more than one HTTP message is needed
by a test like the one for appendix C.5.3.::

    mk_hex <<EOF
    # C.5.1.  First Response
    4803 3330 3258 0770 7269 7661 7465 611d | H.302X.privatea.
    4d6f 6e2c 2032 3120 4f63 7420 3230 3133 | Mon, 21 Oct 2013
    2032 303a 3133 3a32 3120 474d 546e 1768 |  20:13:21 GMTn.h
    7474 7073 3a2f 2f77 7777 2e65 7861 6d70 | ttps://www.examp
    6c65 2e63 6f6d                          | le.com

    # C.5.2.  Second Response
    4803 3330 37c1 c0bf                     | H.307...

    # C.5.3.  Third Response
    88c1 611d 4d6f 6e2c 2032 3120 4f63 7420 | ..a.Mon, 21 Oct
    3230 3133 2032 303a 3133 3a32 3220 474d | 2013 20:13:22 GM
    54c0 5a04 677a 6970 7738 666f 6f3d 4153 | T.Z.gzipw8foo=AS
    444a 4b48 514b 425a 584f 5157 454f 5049 | DJKHQKBZXOQWEOPI
    5541 5851 5745 4f49 553b 206d 6178 2d61 | UAXQWEOIU; max-a
    6765 3d33 3630 303b 2076 6572 7369 6f6e | ge=3600; version
    3d31                                    | =1
    EOF

Once the output is created using the ``mk_*`` functions, the test can finally
run one or both of the ``tst_decode`` and ``tst_encode`` functions. The former
will feed the binary file to  the ``hdecode`` C program and check that the
decoded HTTP message and the dynamic table match the ones declared. The latter
will feed the encoding script to the ``hencode`` C program and check that the
binary output matches the one from the *hexdump* and performs a similar check
for the dynamic table.

Testing edge cases
------------------

Unfortunately examples from the RFC are far from enough to get decent coverage
of the HPACK protocol. The appendices don't even bother showing an update of
the soft limit of a dynamic table, and they all take the happy path. So in
addition to the ``rfc_*`` test cases are the ``hpack_*`` cases that aim at
increasing both HPACK and cashpack coverage in the test suite.

The ``tst_??code`` functions accept two command-line options for test cases
that diverge from the happy and default paths. For instance it is possible to
set the initial size of the dynamic table, as shown in some appendices::

    tst_decode -t 256 # start with a 256B table

It is also possible to expect a decoding or an encoding error, but it also
requires to build empty files for the HTTP message and the dynamic table::

    mk_msg </dev/null
    mk_tbl </dev/null

    tst_encode -r IDX # expect an invalid index

When several header blocks are decoded at once, the size of all blocks are
passed as a comma-separated list. The last size is omitted and instead deduced
from the total size::

    tst_decode -s 70,8, # decodes 3 blocks

In some cases *hexdumps* are not *that* helpful and a binary representation is
a better match. This requirement is covered by another function used by some
tests mostly related to integer encoding::

    mk_bin <<EOF
    00001111 | Use a literal field
    11110001 | to make a 4+ integer
    11111111 | overflow with the
    00000011 | value UINT16_MAX + 1
    EOF

    tst_decode -r INT

Finally, an anonymous hero managed to break invariants in the library by using
American Fuzzy Lop and helped fixing bugs early. Those tests can be found in
the ``afl_fuzz`` script.

The encoding DSL grammar
------------------------

A slightly more interesting example of the encoding DSL usage can be found for
appendix C.6.3.::

    indexed 8
    indexed 65
    dynamic idx 33 huf Mon, 21 Oct 2013 20:13:22 GMT
    indexed 64
    dynamic idx 26 huf gzip
    dynamic idx 55 huf foo=ASDJKHQKBZXOQWEOPIUAXQWEOIU; max-age=3600; version=1

This is a statement-oriented language that is interpreted one line at a time.
Commands and arguments are separated by single spaces to make parsing easier,
and conveniently only header field values can contain spaces but they can only
be last so it works by sheer luck.

.. TODO: add the flush statement to the grammar

::

    encoding-script = 1*( statement LF )

    statement = indexed-field / dynamic-field / literal-field / never-field /
        table-update

    indexed-field = "indexed" SP index
    dynamic-field = "dynamic" SP field-name SP field-value
    literal-field = "literal" SP field-name SP field-value
    never-field   = "never" SP field-name SP field-value
    table-update  = "update" SP size

    index  = number
    size   = number
    number = 1*DIGIT

    field-name = field-index / field-token

    field-index = "idx" SP index
    field-token = ( "str" / "huf" ) SP token
    field-value = ( "str" / "huf" ) SP field-content

See RFC 7230 for undefined labels in the grammar. The ``idx``, ``str`` and
``huf`` tokens announce that their next tokens are expected to be respectively
an index, a string, or a string that should be Huffman-coded.

Interoperability checks
-----------------------

Because HPACK is a protocol, cashpack should be able to work fine with any
other HPACK implementation. For that it uses ``nghttp2`` in places where it
makes sense, but this testing is rather limited at this time. Only decoding
is done, using an ``ngdecode`` C program behaving similarly to ``hdecode``
and it has to be explicitly specified when it is used::

    mk_bin <<EOF
    00111111 | Use a table update
    10000000 | to make a 5+ integer
    10000000 | stupidly packed with
    10000000 | way more bytes than
    10000000 | needed to encode its
    10000000 | rather small value.
    00000000 | It must work regarless.
    EOF

    HDECODE="hdecode ngdecode"
    tst_decode -t 1024

If ``nghttp2`` is not available on your system, the interoperability checks
will be automatically skipped. It is looked up at configure time::

    ./configure
    [...]
    checking for NGHTTP2... yes
    [...]

Compatibility tests may be extended to other HPACK implementations. For that
the main requirements are the ability to probe the dynamic table, enough
control over the coding process and the ability to write ``hencode``-like and
``hdecode``-like programs.

Additional checking
-------------------

Writing software in C can be challenging at times, and even assuming no bugs
in the tool chain it's too easy to corrupt memory or do any kind of fault that
will patiently wait to trigger an error later and fail at a point so remote
that tracking the bug down becomes a nightmare.

One helpful thing is to turn on as many compiler warnings as possible, and
treat them as errors. This is enforced by the build system and won't be as
effective as Rust's compiler for instance, but that removes some classes of
possible errors. At least there's no concurrency in HPACK, so we can also let
compilers do aggressive optimizations and testing all optimization levels with
continuous integration may reveal undefined behaviour in the code.

Going further with undefined behaviour detection, it is possible to build with
ASAN (address sanitizer) and UBSAN (undefined behaviour sanitizer) support for
GCC and clang. If Valgrind is available, its memcheck tool can also be used to
identify undefined behaviour and detect leaks.

Of course all this extra-tooling comes after the very first testing facility
in cashpack: ``assert``. What unit tests often do besides checking computation
results is the verification that invariants are met. Instead of outsourcing
invariant checks, they are closer to potential faults origins: the source code
itself. About 5% of the code is dedicated to that.

Closing words
-------------

There are no unit tests in cashpack, and yet the library had a decent coverage
of 90% at the time of the writing of this documentation. That would be some
average of lines of code, functions and branches coverage if that even means
anything. The code coverage of a test suite doesn't even necessary reflect the
quality of the tests.

It's just a numbers game and some kinds of *wossname* coverage are quite hard
to quantify, like for example the infinite possibilities of decodable input,
or interoperability in the absence of a proper technology compatibility kit.
State is probably one of the hardest thing to cover in general, and setting up
the system under test can be a lot easier with unit testing. It also means
introducing heavy coupling, whereas raising the level of abstraction may allow
testing even if the internals radically change, as it is done with ``nghttp2``
today and may be done for a hypothetical redesign of cashpack or the addition
of more interoperability checks.

Finally, some things can't be tested easily without making shell parts more
complex or less portable by sticking to something like ``bash``. In all cases,
unless I come up with a satisfying solution, I won't automate testing and most
definitely not resort to unit tests.

That being said, Happy Testing!
