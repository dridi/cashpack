The cashpack release process
============================

Administrativa:

- Check for ABI changes and the linker version script
- Bump the version number
- Add a changelog entry to the RPM spec
- Check copyright notices of updated files
- Check git-ignored files after a non-VPATH build
- Check generated headers and manual pages

Local checks:

- Build with GCC, Clang, pcc, afl-gcc and Sparse
- Check with scan-build and cppcheck
- Run the test suite with nghttp2 and Go 1.13+
- Build with gmake and bmake
- Test with bash, dash, ksh, lksh, mksh, yash and zsh
- Build using asan+ubsan, Valgrind and lcov
- Run again with msan and an instrumented nghttp2
- Build an RPM package from a dist archive
- Run rpmlint on both the spec, SRPM and RPM
- Double-check ABI changes with abipkgdiff and readelf

.. cppcheck cheat sheet:
.. --std=c99 --enable=all-except-style-and-information
.. -I/usr/include -Icompiler-include -Irepo-include...

Remote checks:

- Run the almost-comprehensive build matrix on Travis CI
- Trigger an analysis by Coverity Scan
- Build with SunCC
- Build on i686, armv7hl, aarch64, ppc64, ppc64le and s390x
- Prepare release notes on GitHub
- Await Coverity Scan results

Release:

- Create a tag ``v${VERSION}``
- Upload the dist archive
- Publish the release

Estimated time if nothing goes wrong: 1h.
