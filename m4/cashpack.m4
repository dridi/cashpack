# Copyright (c) 2016 Dridi Boukelmoune
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

# CASHPACK_SANITY_CHECK
# ---------------------
AC_DEFUN([CASHPACK_SANITY_CHECK], [

	dnl hexcheck
	AC_MSG_CHECKING([for RFC 7541-compatible hexdumps])

	if ! "$srcdir"/tst/hexcheck 2>/dev/null
	then
		AC_MSG_RESULT([no])
		"$srcdir"/tst/hexcheck
		AC_MSG_FAILURE([hexdumps fail sanity check])
	fi

	AC_MSG_RESULT([yes])

	dnl bincheck
	AC_MSG_CHECKING([for working bindumps to hexdumps conversions])

	if ! "$srcdir"/tst/bincheck 2>/dev/null
	then
		AC_MSG_RESULT([no])
		"$srcdir"/tst/bincheck
		AC_MSG_FAILURE([bindumps fail sanity check])
	fi

	AC_MSG_RESULT([yes])

	dnl Mutual exclusivity of additional checkers
	cashpack_options="$(
		echo "$with_memcheck$with_asan$with_ubsan$with_lcov" |
		awk -F yes '{print NF - 1}'
	)"

	test "$cashpack_options" -gt 1 &&
	AC_MSG_FAILURE([Valgrind, ASAN, UBSAN and lcov support can't be combined])

])

# CASHPACK_PROG_RST2MAN
-----------------------
AC_DEFUN([CASHPACK_PROG_RST2MAN], [

	AC_CHECK_PROGS(RST2MAN, [rst2man.py rst2man], [true])
	AC_SUBST([RST2MAN])

])

# CASHPACK_PROG_UNCRUSTIFY
# ------------------------
AC_DEFUN([CASHPACK_PROG_UNCRUSTIFY], [

	UNCRUSTIFY_OPTS="-c '\$(srcdir)/uncrustify.cfg' -q -l C --no-backup"

	AC_CHECK_PROGS(UNCRUSTIFY, [uncrustify], [true])
	test "$UNCRUSTIFY" = true && UNCRUSTIFY_OPTS=

	AC_SUBST([UNCRUSTIFY])
	AC_SUBST([UNCRUSTIFY_OPTS])

])

# CASHPACK_LIB_NGHTTP2
# --------------------
AC_DEFUN([CASHPACK_LIB_NGHTTP2], [

	PKG_CHECK_MODULES([NGHTTP2],
		[libnghttp2],
		[NGHTTP2=yes],
		[NGHTTP2=no])

	AC_SUBST([NGHTTP2])
	AM_CONDITIONAL([HAVE_NGHTTP2], [test "$NGHTTP2" = yes])

])

# CASHPACK_WITH_MEMCHECK
# ----------------------
AC_DEFUN([CASHPACK_WITH_MEMCHECK], [

	AC_CHECK_PROGS(VALGRIND, [valgrind], [no])

	AC_ARG_WITH([memcheck],
		AS_HELP_STRING(
			[--with-memcheck],
			[Run the test suite with Valgrind]),
		[MEMCHECK="$withval"],
		[MEMCHECK=OFF])

	test "$MEMCHECK" = yes && MEMCHECK=ON

	test "$MEMCHECK" = ON -a "$VALGRIND" = no &&
	AC_MSG_FAILURE([Valgrind is required with memcheck])

	AC_SUBST([MEMCHECK])

])

# _CASHPACK_ASAN
# --------------
AC_DEFUN([_CASHPACK_ASAN], [

	CFLAGS="$CFLAGS -fsanitize=address"
	AC_CHECK_LIB(
		[asan],
		[__asan_address_is_poisoned],
		[LIBS="$ac_check_lib_save_LIBS"])

])

# CASHPACK_WITH_ASAN
# ------------------
AC_DEFUN([CASHPACK_WITH_ASAN], [

	AC_ARG_WITH([asan],
		AS_HELP_STRING(
			[--with-asan],
			[Build binaries with address sanitizer]),
		[_CASHPACK_ASAN],
		[])

])

# _CASHPACK_UBSAN
# ---------------
AC_DEFUN([_CASHPACK_UBSAN], [

	CFLAGS="$CFLAGS -fsanitize=undefined"
	AC_CHECK_LIB([ubsan],
		[__ubsan_handle_add_overflow],
		[LIBS="$ac_check_lib_save_LIBS"])

])

# CASHPACK_WITH_UBSAN
# -------------------
AC_DEFUN([CASHPACK_WITH_UBSAN], [

	AC_ARG_WITH([ubsan],
		AS_HELP_STRING(
			[--with-ubsan],
			[Build binaries with undefined sanitizer]),
		[_CASHPACK_UBSAN],
		[])

])

# _CASHPACK_LCOV
# --------------
AC_DEFUN([_CASHPACK_LCOV], [

	AC_CHECK_PROGS(LCOV, [lcov], [no])
	test "$LCOV" = no &&
	AC_MSG_FAILURE([Lcov is required for code coverage])

	AC_CHECK_PROGS(GENHTML, [genhtml], [no])
	test "$GENHTML" = no &&
	AC_MSG_FAILURE([Lcov is missing genhtml for reports generation])

	LCOV_RULES="

lcov: all
	@\$(LCOV) -z -d .
	@\$(MAKE) \$(AM_MAKEFLAGS) -k check
	@\$(LCOV) -c -o tst.info -d tst
	@\$(LCOV) -c -o lib.info -d lib
	@\$(LCOV) -a tst.info -a lib.info -o raw.info
	@\$(LCOV) -r raw.info '/usr/*' -o cashpack.info
	@\$(GENHTML) -o lcov cashpack.info
	@echo file://\$(abs_builddir)/lcov/index.html

lcov-clean:

clean: lcov-clean
	@find \$(abs_builddir) -depth '(' \
		-name '*.gcda' -o \
		-name '*.gcov' -o \
		-name '*.gcno' -o \
		-name '*.info' \
		')' -delete
	@rm -rf \$(abs_builddir)/lcov/

.PHONY: lcov lcov-clean

"

	CPPFLAGS="$CPPFLAGS -DNDEBUG"
	CFLAGS="$CFLAGS -O0 -g -fprofile-arcs -ftest-coverage"
	LDFLAGS="$LDFLAGS -lgcov"

	AC_SUBST([LCOV])
	AC_SUBST([GENHTML])
	AC_SUBST([LCOV_RULES])
	m4_ifdef([_AM_SUBST_NOTMAKE], [_AM_SUBST_NOTMAKE([LCOV_RULES])])

])

# CASHPACK_WITH_LCOV
# ------------------
AC_DEFUN([CASHPACK_WITH_LCOV], [

	AC_ARG_WITH([lcov],
		AS_HELP_STRING(
			[--with-lcov],
			[Measure test suite code coverage with lcov]),
		[_CASHPACK_LCOV],
		[])

])
