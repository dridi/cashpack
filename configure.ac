# Copyright (c) 2016-2020 Dridi Boukelmoune
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

AC_INIT([cashpack], [0.4], [https://github.com/dridi/cashpack/issues])
AC_COPYRIGHT([Copyright (c) 2016-2020 Dridi Boukelmoune])

AC_PREREQ([2.63])
AC_REVISION([revision 2])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([lib/hpack.c])
AC_CONFIG_AUX_DIR([build-aux])

# NB: This project targets standards C99 and POSIX.1-2008,
# but some systems suck when it comes to feature detection
# and want system extensions for functions like getline.
AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE([1.12 foreign])
AM_SILENT_RULES([no])
AM_EXTRA_RECURSIVE_TARGETS([fuzz])
AM_SANITY_CHECK

LT_INIT
LT_PREREQ([2.2.6])

AC_PROG_SED
AC_PROG_AWK
AC_PROG_CC_C99
AC_C_STRINGIZE

gl_LD_VERSION_SCRIPT

CASHPACK_ARG_ENABLE([docs], [no])
CASHPACK_ARG_ENABLE([warnings], [no])
CASHPACK_ARG_ENABLE([hardening], [no])
CASHPACK_ARG_ENABLE([memcheck], [no])
CASHPACK_ARG_ENABLE([fuzzer], [no])
CASHPACK_ARG_ENABLE([asan], [no])
CASHPACK_ARG_ENABLE([msan], [no])
CASHPACK_ARG_ENABLE([ubsan], [no])
CASHPACK_ARG_ENABLE([gcov], [no])
CASHPACK_ARG_ENABLE([lcov], [no])
CASHPACK_COND_PROG([HEXDUMP], [hexdump], [for examples and tests])
CASHPACK_COND_PROG([GO], [go], [for compatibility tests])
CASHPACK_COND_MODULE([NGHTTP2], [libnghttp2])

# Standards compliance
CASHPACK_CHECK_CFLAGS([CFLAGS], [
	-pedantic
	-std=c99
	-D_POSIX_C_SOURCE=200809L
])

# Documentation
AM_CONDITIONAL([DOCS], [test "$enable_docs" != "no"])
test "$enable_docs" != "no" &&
CASHPACK_CHECK_PROG([RST2MAN], [rst2man rst2man.py], [required to build docs])

# Dynamic analysis
test "$enable_asan" != no &&
CASHPACK_ADD_CFLAGS([CFLAGS], [-fno-omit-frame-pointer -fsanitize=address])

test "$enable_msan" != no &&
CASHPACK_ADD_CFLAGS([CFLAGS], [-fsanitize=memory])

test "$enable_ubsan" != no &&
CASHPACK_ADD_CFLAGS([CFLAGS], [-fsanitize=undefined])

AM_CONDITIONAL([FUZZ], [test "$enable_fuzzer" != "no"])

if test "$enable_memcheck" != no
then
	CASHPACK_CHECK_PROG([VALGRIND], [valgrind],
		[required by --enable-memcheck])
	AC_SUBST([MEMCHECK], [yes])
fi

# Coverage
AM_CONDITIONAL([GCOV_ENABLED], [test "$enable_gcov" != no])
AM_CONDITIONAL([LCOV_ENABLED], [test "$enable_lcov" != no])

if test "$enable_gcov$enable_lcov" != nono
then
	CASHPACK_CHECK_PROG([GCOV], [gcov], [only works with GCC])
	CASHPACK_CHECK_PROG([CPIO], [cpio], [required by gcov])
	CASHPACK_ADD_CFLAGS([CFLAGS], [
		-O0 -g
		-fno-inline
		--coverage
		-DNDEBUG
	])
	enable_warnings=no
fi

if test "$enable_lcov" != no
then
	CASHPACK_CHECK_PROG([LCOV], [lcov], [only works with GCC])
	CASHPACK_CHECK_PROG([GENHTML], [genhtml], [required by lcov])

	if "$LCOV" --help | grep -q -e --config-file
	then
		LCOV_OPTS="--config-file \$(srcdir)/lcovrc"
		LCOV="$LCOV $LCOV_OPTS"
		GENHTML="$GENHTML $LCOV_OPTS"
	fi

	LCOV="$LCOV --gcov-tool '[\$](abs_top_builddir)/tst/cov.sh'"
fi

# Warnings
if test "$enable_warnings" != no
then
	# FreeBSD's WARNS level 6
	CASHPACK_CHECK_CFLAGS([CFLAGS], [
		-Werror
		-Wall
		-W
		-Wstrict-prototypes
		-Wmissing-prototypes
		-Wpointer-arith
		-Wreturn-type
		-Wcast-qual
		-Wwrite-strings
		-Wswitch
		-Wshadow
		-Wunused-parameter
		-Wcast-align
		-Wchar-subscripts
		-Winline
		-Wnested-externs
		-Wredundant-decls
		-Wold-style-definition
		-Wmissing-variable-declarations
	])

	# Other desirable warnings
	CASHPACK_CHECK_CFLAGS([CFLAGS], [
		-Wextra
		-Wmissing-declarations
		-Wredundant-decls
		-Wsign-compare
		-Wunused-result
		-Wformat-security
	])

	# Clang unleashed-ish
	CASHPACK_CHECK_CFLAGS([CFLAGS], [
		-Weverything
		-Wno-padded
		-Wno-string-conversion
		-Wno-covered-switch-default
		-Wno-disabled-macro-expansion
	])

	# SunCC-specific warnings
	CASHPACK_CHECK_CFLAGS([CFLAGS], [
		-errwarn=%all
		-errtags=yes
	])

	# sparse(1) warnings
	CASHPACK_CHECK_CFLAGS([CFLAGS], [
		-Wsparse-all
		-Wsparse-error
	])

	# Keep examples simple enough
	CASHPACK_CHECK_CFLAGS([EXAMPLE_CFLAGS], [
		-Wno-conversion
		-Wno-format-nonliteral
		-Wno-missing-noreturn
		-Wno-switch-enum
	])

	# Give some slack to the test suite, it covers nasty stuff
	CASHPACK_CHECK_CFLAGS([TEST_CFLAGS], [
		-Wno-assign-enum
		-Wno-conversion
		-Wno-format-nonliteral
		-Wno-sign-conversion
		-Wno-switch-enum
		-Wno-unreachable-code-return
	])

	if test -z "${CC%*clang*}"
	then
		CASHPACK_CHECK_CFLAGS([CFLAGS], [-Wno-error=implicit-fallthrough])
	fi
fi

# Hardening
if test "$enable_hardening" != no
then
	CASHPACK_CHECK_CFLAGS([CFLAGS], [
		-fstack-clash-protection
		-fstack-protector
	])
fi

AC_SUBST([EXAMPLE_CFLAGS], ["$CFLAGS $EXAMPLE_CFLAGS"])
AC_SUBST([TEST_CFLAGS], ["$CFLAGS $TEST_CFLAGS"])

# Test suite
AC_MSG_CHECKING([for RFC 7541-compatible hexdumps])
if ! "$srcdir"/tst/hexcheck 2>/dev/null
then
	AC_MSG_RESULT([no])
	"$srcdir"/tst/hexcheck
	AC_MSG_FAILURE([hexdumps fail sanity check])
fi
AC_MSG_RESULT([yes])

AC_MSG_CHECKING([for working bindumps to hexdumps conversions])
if ! "$srcdir"/tst/bincheck 2>/dev/null
then
	AC_MSG_RESULT([no])
	"$srcdir"/tst/bincheck
	AC_MSG_FAILURE([bindumps fail sanity check])
fi
AC_MSG_RESULT([yes])

test "$GO" != no &&
CASHPACK_GOLANG_PREREQ([1.13])

# The end
AC_CONFIG_FILES([
	Makefile
	cashpack.spec
	gen/Makefile
	inc/Makefile
	lib/Makefile
	lib/cashpack.pc
	man/Makefile
	man/cashpack.3.rst
	man/hpack_encode.3.rst
	man/hpack_error.3.rst
	tst/Makefile
])

AC_OUTPUT
