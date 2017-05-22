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
setting up a temp working directory. It also abstracts away most of the setup
so that the test cases contain as little boilerplate as possible.

The C programs serve as bindings between the C library and the shell scripts,
offering a convenient interface even for edge cases. Reaching a particular
line or branch from outside the library should always be straightforward given
the relative simplicity of HPACK.

It also means that the test suite cannot cheat the type system and reach
private functions directly. All test cases go through the public API, so every
single test is relevant for production, with a degree of relevance varying on
the test subject (silly or rare edge cases may be of lower importance).

The typical structure of a test case is the following::

    . "$(dirname "$0")"/common.sh

    _ ---------------
    _ Test case title
    _ ---------------

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

Coverage of the HPACK protocol
------------------------------

Unfortunately examples from the RFC are far from enough to get decent coverage
of the HPACK protocol. The appendices don't even bother showing an update of
the soft limit of a dynamic table, and all examples take the happy path.

For that matter, sections describing decoding errors or expected behavior have
their own test cases in addition to the examples from the appendix C. This
should be enough to demonstrate cashpack's compliance.

Testing edge cases
------------------

Being compliant is one thing, but cashpack has its peculiar architecture and
needs coverage of its own. So in addition to the ``rfc_*`` test cases are the
``hpack_*`` cases that aim at increasing cashpack coverage in the test suite.

The ``tst_??code`` functions accept two command-line options for test cases
that diverge from the happy and default paths. For instance it is possible to
set the initial size of the dynamic table, as shown in some appendices::

    tst_decode --table-size 256 # start with a 256B table

It is also possible to expect a decoding or an encoding error, but it also
requires to build empty files for the HTTP message and the dynamic table::

    mk_msg </dev/null
    mk_tbl </dev/null

    mk_enc <<EOF
    indexed 9001
    EOF

    tst_encode --expect-error IDX # expect an invalid index

For the specific needs of decoding, another option handled by ``hdecode`` only
can lower the decoding buffer below its default value::

    tst_decode --buffer-size 256 --expect-error BIG

When several header blocks are decoded at once, the size of all blocks are
passed as a comma-separated list. The last size is omitted and instead deduced
from the total size::

    tst_decode --decoding-spec d70,d8, # decodes 3 blocks

This list of sizes can also contain dynamic table sizes when they are resized
out of band, like HTTP/2 settings. In this case the 'd' size prefix's replaced
by 'r'. Partial blocks may be decoded, in this case the prefix is 'p'. It is
also possible to skip a message too big using 's' and 'S' instead of 'd' and
'p'. The character 'a' aborts, more on that later.

In some cases *hexdumps* are not *that* helpful and a binary representation is
a better match. This requirement is covered by another function used by some
tests mostly related to integer encoding::

    mk_bin <<EOF
    00001111 | Use a literal field
    11110001 | to make a 4+ integer
    11111111 | overflow with the
    00000011 | value UINT16_MAX + 1
    EOF

    tst_decode --expect-error INT

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
appear last in a statement: it worked by sheer luck \o/.

::

    encoding-script = 1*( statement )

    statement = block-statement / resize / update / abort

    block-statement = 1*( header-statement LF ) flush-statement
    flush-statement = send / push

    header-statement = indexed-field / dynamic-field / literal-field /
        never-field

    indexed-field = "indexed" SP index
    dynamic-field = "dynamic" SP field-name SP field-value
    literal-field = "literal" SP field-name SP field-value
    never-field   = "never" SP field-name SP field-value
    corrupt-field = "corrupt" LF
    send          = "send" LF
    push          = "push" LF
    resize        = "resize" SP size LF
    update        = "update" SP size LF
    abort         = "abort" LF

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

Writing hexadecimal soup
------------------------

Copying hexadecimal sequences from the RFC's appendices is very easy, fair
enough. Testing encoding is also fairly easy because the encoding DSL is
literally straightforward, and simple to write. But covering the appendices
don't even come close to reaching a decent coverage, so most of the test
suite had to be written by hand. That includes hexadecimal sequences, and
among them packed integers and Huffman strings.

The most common solution was to write the encoding test case by hand, and
copy the *hexdump* as-is for the decoding test. This introduces the risk of
coordinated bugs where both cases are wrong but they look OK to each other.
However the encoding is a lot simpler and less error-prone than decoding,
which is essentially the same as serializing vs parsing.

So the risk is low, but not zero. In the case of Huffman coding, the test
suite survived a complete rewrite without flinching. And since Huffman coding
exercises most of HPACK features, the risk for coordinated bugs is even lower.
Interoperability checks with other HPACK implementations lower the risk even
further (see below).

For other tests, mostly the tricky edge cases, the hexadecimal is hand written
and commented. And a simple command line utility called ``hpiencode`` exists
in the source tree to avoid making (inevitable) mistakes::

    ./lib/hpiencode HUF 123
    fb
    ./lib/hpiencode UPD 4096
    3fe11f

As a side note, some tests involving lengthy strings were made easily possible
thanks to two characters: ``'0'`` and ``'3'``. ``'3'`` has the hexadecimal
code ``33`` in ASCII, it can be used to both represent itself as a character
or half of itself in hexadecimal.

``'0'`` on the other hand has the Huffman code ``0`` on 5 bits so the Huffman
string ``"00000000"`` can be represented as ten zeros in hexadecimal. It's
only a simple matter of basic arithmetics to get reliable long strings for
some of the edge cases!

Interoperability checks
-----------------------

Because HPACK is a protocol, cashpack should be able to work fine with any
other HPACK implementation. For that it used ``nghttp2`` in places where it
makes sense, but this is now done systematically using an ``ngdecode`` C
program that behaves similarly to ``hdecode``. This is also useful because in
some areas the spec is not always strict::

    mk_bin <<EOF
    00111111 | Use a table update
    10000000 | to make a 5+ integer
    10000000 | stupidly packed with
    10000000 | way more bytes than
    10000000 | needed to encode its
    10000000 | rather small value.
    10000000 | For cashpack it must
    00000000 | work regardless.
    EOF

    tst_decode --table-size 1024

Later on, a similar ``godecode`` program was added to challenge cashpack with
Go's native HPACK stack. Go ships with HTTP/2 since Go 1.6, and if ``nghttp2``
or Go is not available on your system, the relevant interoperability checks
will be automatically skipped. It is looked up at configure time::

    ./configure
    [...]
    checking for NGHTTP2... yes
    checking for golang >= 1.7... yes
    [...]

Some of ``nghttp2`` tests fail and are deactivated. It may be fixed on newer
versions or handled at the HTTP/2 level. It doesn't necessary mean that the
library is wrong.

Compatibility tests may be extended to other HPACK implementations. For that
the main requirements are the ability to probe [1]_ the dynamic table, enough
control over the coding process and the ability to write ``hencode``-like and
``hdecode``-like programs. This has yet to happen for encoding.

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
ASAN (address sanitizer), MSAN (memory sanitizer) or UBSAN (undefined behavior
sanitizer) support for GCC and Clang. If Valgrind is available, its memcheck
tool can also be used to identify undefined behaviour and detect leaks.

Of course all this extra-tooling comes after the very first testing facility
in cashpack: ``assert``. What unit tests often do besides checking computation
results is the verification that invariants are met. Instead of outsourcing
invariant checks, they are closer to potential faults origins: the source code
itself. About 5% of the whole C code base is dedicated to that, but it's about
8% for the library itself.

Portability
-----------

Portability is an important factor for cashpack and although it won't directly
contribute to the test suite, it is actually related. As stated above, using
sanitizers or different optimization levels can help check against undefined
behavior. Switching compilers can also reveal undefined behavior, especially
for a language as weak as C. Running without asserts (eg. with ``lcov``) is
also a good way to spot coding mistakes.

That's where Travis CI fits in the picture. The cashpack project is integrated
with a build matrix covering optimizations, sanitizers and even Valgrind. All
of that with both GCC and Clang, with older versions. Travis CI is a bit short
sighted when it comes to continuous integration, trying to solve a too narrow
CI problem space, but at the same time it covers a great deal of needs, all of
that for free!

Speaking of coverage, Travis CI made it possible in a rather convenient way to
publish code coverage reports with codecov.io and do static analysis using
Coverity Scan. Other static analysis tools were evaluated but most of the time
yielding far too many false-positives. With the exception of ``cppcheck(1)``
and Clang's ``scan-build(1)`` that do a great job too.

So Travis CI helps on both continuous integration and compiler portability.
But it also helps check the portability of the Shell test suite. The default
shell on Ubuntu/Debian is ``dash(1)``, which is as POSIX as a shell can get.
At release time it is tested against other POSIX-compatible shells, while
``bash(1)`` is the daily driver. The other shells include in alphabetic order:
``ksh`` (``ksh93``), ``lksh``, ``mksh``, ``yash`` and ``zsh``.

For instance, to run the test suite with ``yash``::

    make check LOG_COMPILER=yash

Finally, architecture portability. cashpack is intended for embedded systems
but would work fine with "regular" systems too. However it does not target
8-bit micro-controllers or any similar *dedicated* devices, but actual general
purpose 32- or 64-bit CPUs: common enough in the embedded space.

Once again, C being C you may get different results on different platforms if
you inadvertently rely on undefined behavior. Thanks to resources provided by
the Fedora Project, a lot of CPU architectures are used to *manually* run the
test suite. It would be interesting too to link against ``libc``\s other than
``glibc``.

+----------+------------+------------+-----------+-------+-----------+-----------+
| Compiler | GCC        | Clang      | pcc       | SunCC | afl-gcc   | Sparse    |
+----------+------------+------------+-----------+-------+-----------+-----------+
| Arch     | Targets                                                             |
+==========+============+============+===========+=======+===========+===========+
| x86_64   | FreeBSD,   | FreeBSD,   | GNU/Linux | SunOS | GNU/Linux | GNU/Linux |
|          | GNU/Linux, | GNU/Linux, |           |       |           |           |
|          | OSX,       | OSX        |           |       |           |           |
|          | SunOS      |            |           |       |           |           |
+----------+------------+------------+-----------+-------+-----------+-----------+
| i686     | GNU/Linux  | —          | —         | —     | —         | —         |
+----------+------------+------------+-----------+-------+-----------+-----------+
| armv7hl  | GNU/Linux  | —          | —         | —     | —         | —         |
+----------+------------+------------+-----------+-------+-----------+-----------+
| aarch64  | GNU/Linux  | —          | —         | —     | —         | —         |
+----------+------------+------------+-----------+-------+-----------+-----------+
| ppc64    | GNU/Linux  | —          | —         | —     | —         | —         |
+----------+------------+------------+-----------+-------+-----------+-----------+
| ppc64le  | GNU/Linux  | —          | —         | —     | —         | —         |
+----------+------------+------------+-----------+-------+-----------+-----------+
| s390x    | GNU/Linux  | —          | —         | —     | —         | —         |
+----------+------------+------------+-----------+-------+-----------+-----------+

cashpack works fine on all platforms where it could be tested. It did not
however compile with the Portable C Compiler unless optimizations were
disabled (which looks like a bug in pcc). Sparse and afl-gcc (American Fuzzy
Lop GCC) aren't compilers but an additional source of warnings on top of GCC.

Reporting
---------

Writing test cases is one side of the coin, reporting also takes an important
part in the testing process. cashpack relies on ``automake`` for building and
testing, and the default test runner meets enough requirements:

- process-based testing
- parallel execution
- individual log files
- global log file for failures

So when tests are failing, a ``test-suite.log`` should contain the useful bits
to understand the failure. If a debugger is needed to make progress, that's a
sign that the test suite doesn't report enough and it's usually a good time to
improve it.

On Travis CI, an old version of ``automake`` is used in the Ubuntu 12.04 LTS
containers, so the contents of ``test-suite.log`` can be found directly in the
console's log. This behavior can be brought back in more recent versions of
``automake`` by adding ``AUTOMAKE_OPTIONS = serial-tests`` to the relevant
``Makefile``.

The cashpack test suite itself logs useful information, like the programs that
get executed, their results, and when an assert triggers a dump of the HPACK
data structure.

Here is a passing test log::

    -----------------------
    TEST: Invalid character
    -----------------------
    hpack_decode: ./hdecode --expect-error CHR
    main: hpack result: Invalid character (-7)
    hpack_decode: ./ngdecode --expect-error CHR
    main: nghttp2 result: Header compression/decompression error (-523)
    ----------------------------------------------------------
    TEST: Parse a Huffman string longer than the decode buffer
    ----------------------------------------------------------
    hpack_decode: ./hdecode
    hpack_decode: ./ngdecode
    hpack_encode: ./hencode
    ----------------------------------------------------------
    TEST: Decode a long Huffman string with invalid characters
    ----------------------------------------------------------
    hpack_decode: ./hdecode --expect-error CHR
    main: hpack result: Invalid character (-7)
    hpack_decode: ./ngdecode --expect-error CHR
    main: nghttp2 result: Header compression/decompression error (-523)
    PASS hpack_huf (exit status: 0)

The different test cases stand out thanks to their title, and each case has
one or more checks to perform. If a check appears to run fine, its output is
then challenged (``diff -u``) against the expected results::

    FAIL: rfc7541_c_6_3
    ===================

    hpack_decode: ./hdecode --decoding-spec d54,d8, --table-size 256
    --- cashpack.Eif6kv7m/dec_out   2016-05-15 12:40:25.888082395 +0200
    +++ cashpack.Eif6kv7m/out       2016-05-15 12:40:25.889082406 +0200
    @@ -18,6 +18,6 @@
     Dynamic Table (after decoding):

     [  1] (s =  98) set-cookie: foo=ASDJKHQKBZXOQWEOPIUAXQWEOIU; max-age=3600; version=1
    -[  2] (s =  52) content-encoding: gzip
    +[  2] (s =  52) content-encoding: date
     [  3] (s =  65) date: Mon, 21 Oct 2013 20:13:22 GMT
           Table size: 215
    FAIL rfc7541_c_6_3 (exit status: 1)

Sometimes, it might be useful to inspect the output files in the tests temp
directories. Keeping those files around is only a matter of adding a parameter
to the ``make`` command line::

    make check KEEP_TMP=yes

Finally, if an invariant is not met and triggers an assert, a dump of the data
structure is printed in the standard error. The C programs used in testing may
abort instead of doing proper error handling for convenience. In both cases
the output may look like this::

    FAIL: rfc7541_c_6_3
    ===================

    hpack_decode: ./hdecode --decoding-spec d54,d8, --table-size 256
    lt-hdecode: tst/hdecode.c:237: main: Assertion `!"Incomplete code"' failed.
    *hp = 0x256dc20 {
        .magic = ab0e3218
        [... gory details ...]
        .tbl = 0x256dce0 <<EOF
        00000000  39 2b 58 e4 00 00 00 00  00 00 00 00 00 00 00 00  |9+X.............|
        00000010  0a 00 38 00 00 00 00 00  00 00 00 00 00 00 73 65  |..8...........se|
        00000020  74 2d 63 6f 6f 6b 69 65  00 66 6f 6f 3d 41 53 44  |t-cookie.foo=ASD|
        00000030  4a 4b 48 51 4b 42 5a 58  4f 51 57 45 4f 50 49 55  |JKHQKBZXOQWEOPIU|
        00000040  41 58 51 57 45 4f 49 55  3b 20 6d 61 78 2d 61 67  |AXQWEOIU; max-ag|
        00000050  65 3d 33 36 30 30 3b 20  76 65 72 73 69 6f 6e 3d  |e=3600; version=|
        00000060  31 00 39 2b 58 e4 00 00  00 00 62 00 00 00 00 00  |1.9+X.....b.....|
        00000070  00 00 10 00 04 00 00 00  00 00 00 00 00 00 00 00  |................|
        00000080  63 6f 6e 74 65 6e 74 2d  65 6e 63 6f 64 69 6e 67  |content-encoding|
        00000090  00 67 7a 69 70 00 39 2b  58 e4 00 00 00 00 34 00  |.gzip.9+X.....4.|
        000000a0  00 00 00 00 00 00 04 00  1d 00 00 00 00 00 00 00  |................|
        000000b0  00 00 00 00 64 61 74 65  00 4d 6f 6e 2c 20 32 31  |....date.Mon, 21|
        000000c0  20 4f 63 74 20 32 30 31  33 20 32 30 3a 31 33 3a  | Oct 2013 20:13:|
        000000d0  32 32 20 47 4d 54 00                              |22 GMT.|
        EOF
    }
    tst/common.sh: line 57: 16044 Aborted                 (core dumped) "$@"
    FAIL rfc7541_c_6_3 (exit status: 134)

It is also possible to manually abort the test suite at any given time using
either the decoding spec for ``hdecode`` or the DSL for ``hencode``. This way
it becomes possible to get the data structure dump at the desired step of the
coding process.

If that's really not enough, then all hail the mighty interactive debugger.

Closing words
-------------

There are no unit tests in cashpack, and yet the library had a decent coverage
of 90% [2]_ at the time of the writing of this documentation's first revision.
That would be some average of lines of code functions and branches coverage if
that even means anything. The code coverage of a test suite doesn't even
necessary reflect the quality of the tests.

It's just a numbers game and some kinds of *wossname* coverage are quite hard
to quantify, like for example the infinite possibilities of decodable input,
or interoperability in the absence of a proper technology compatibility kit.

State is probably one of the hardest things to cover in general, and setting
up the system under test can be a lot easier with unit testing. It also means
introducing heavy coupling, whereas raising the level of abstraction may allow
testing even if the internals radically change, as it is done with ``nghttp2``
today and may be done for a (not so [3]_) hypothetical redesign of cashpack or
the addition of more interoperability checks [4]_.

Finally, some things can't be tested easily without making shell parts more
complex or less portable by sticking to something like ``bash``. In all cases,
unless I come up with a satisfying solution, I won't automate testing and most
definitely not resort to unit tests.

That being said, Happy Testing!

.. [1] The test suite can now skip the dynamic table checks when unavailable
       as it is sadly the case for Go
.. [2] The coverage peaked at 99% for the library and should stay there
.. [3] The transition from zero-copy to single-copy was painless with almost
       no changes to the test suite despite many changes in the library
.. [4] Go's HPACK implementation passed all tests right away
