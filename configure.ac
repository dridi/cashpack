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

AC_INIT([cashpack], [0.2], [https://github.com/dridi/cashpack/issues])
AC_COPYRIGHT([Copyright (c) 2016 Dridi Boukelmoune])

AC_PREREQ([2.63])
AC_REVISION([revision 1])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([lib/hpack.c])
AC_CONFIG_AUX_DIR([build-aux])

AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE([1.11 foreign])
AM_SILENT_RULES([yes])
AM_SANITY_CHECK

LT_INIT
LT_PREREQ([2.2.6])

AC_PROG_SED
AC_PROG_AWK
AC_PROG_CC_C99
AC_C_STRINGIZE

gl_LD_VERSION_SCRIPT

CASHPACK_CHECK_FLAGS

CASHPACK_LIB_NGHTTP2
CASHPACK_CHECK_GOLANG
CASHPACK_PROG_HEXDUMP
CASHPACK_PROG_RST2MAN
CASHPACK_PROG_UNCRUSTIFY
CASHPACK_WITH_MEMCHECK
CASHPACK_WITH_ASAN
CASHPACK_WITH_MSAN
CASHPACK_WITH_UBSAN
CASHPACK_WITH_LCOV
CASHPACK_ENABLE_DOCS
CASHPACK_SANITY_CHECK

AC_CONFIG_FILES([
	Makefile
	cashpack.spec
	gen/Makefile
	inc/Makefile
	lib/Makefile
	lib/cashpack.pc
	man/Makefile
	tst/Makefile
])

AC_OUTPUT
