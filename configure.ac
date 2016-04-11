AC_INIT([cashpack], [0.1], [https://github.com/dridi/cashpack/issues])
AC_COPYRIGHT([Copyright (c) 2015-2016 Dridi Boukelmoune])

AC_PREREQ([2.63])
AC_REVISION([revision 1])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([lib/hpack.c])

AM_INIT_AUTOMAKE([foreign])
AM_SILENT_RULES([yes])
AM_SANITY_CHECK

LT_INIT
LT_PREREQ([2.2.6])

CASHPACK_PROG_RST2MAN
CASHPACK_PROG_UNCRUSTIFY
CASHPACK_WITH_MEMCHECK
CASHPACK_WITH_ASAN
CASHPACK_WITH_UBSAN
CASHPACK_WITH_LCOV
CASHPACK_SANITY_CHECK

AC_CONFIG_FILES([
	Makefile
	gen/Makefile
	inc/Makefile
	lib/Makefile
	man/Makefile
	tst/Makefile
])

AC_OUTPUT
