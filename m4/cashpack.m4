# Copyright (c) 2016-2018 Dridi Boukelmoune
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

# CASHPACK_ARG_ENABLE(FEATURE, DEFAULT)
# ------------------------------------
AC_DEFUN([CASHPACK_ARG_ENABLE], [dnl
	AC_ARG_ENABLE([$1],
		[AS_HELP_STRING([--enable-$1], [enable $1 (default is $2)])],
		[],
		[enable_[]$1=$2])
])

# _CASHPACK_CHECK_CFLAGS
# ---------------------
AC_DEFUN_ONCE([_CASHPACK_CHECK_CFLAGS], [

cashpack_check_cflag() {
	cashpack_save_CFLAGS=$CFLAGS
	CFLAGS="$cashpack_chkd_cflags $[]1 $cashpack_orig_cflags"

	mv confdefs.h confdefs.h.orig
	touch confdefs.h

	AC_MSG_CHECKING([whether the compiler accepts $[]1])
	AC_RUN_IFELSE(
		[AC_LANG_SOURCE([int main(void) { return (0); }])],
		[cashpack_result=yes],
		[cashpack_result=no])
	AC_MSG_RESULT($cashpack_result)

	rm confdefs.h
	mv confdefs.h.orig confdefs.h

	CFLAGS=$cashpack_save_CFLAGS

	test "$cashpack_result" = yes
}

cashpack_check_cflags() {
	for _cflag
	do
		if cashpack_check_cflag $_cflag
		then
			cashpack_chkd_cflags="$cashpack_chkd_cflags $_cflag"
		fi
	done
}

])

# _CASHPACK_INIT_CFLAGS(VARIABLE)
# ------------------------------
AC_DEFUN([_CASHPACK_INIT_CFLAGS], [dnl
if test -z "$cashpack_orig_cflags_$1"
then
	cashpack_orig_cflags_$1="orig:$$1"
	cashpack_chkd_cflags_$1=
fi
])

# CASHPACK_CHECK_CFLAGS(VARIABLE, CFLAGS)
# --------------------------------------
AC_DEFUN([CASHPACK_CHECK_CFLAGS], [dnl
AC_REQUIRE([_CASHPACK_CHECK_CFLAGS])dnl
{
_CASHPACK_INIT_CFLAGS([$1])dnl
cashpack_orig_cflags=${cashpack_orig_cflags_$1#orig:}
cashpack_chkd_cflags=${cashpack_chkd_cflags_$1# }
cashpack_check_cflags m4_normalize([$2])
$1="$cashpack_chkd_cflags $cashpack_orig_cflags"
cashpack_chkd_cflags_$1=$cashpack_chkd_cflags
}
])

# CASHPACK_ADD_CFLAGS(VARIABLE, CFLAGS)
# ------------------------------------
AC_DEFUN([CASHPACK_ADD_CFLAGS], [dnl
AC_REQUIRE([_CASHPACK_CHECK_CFLAGS])dnl
{
_CASHPACK_INIT_CFLAGS([$1])dnl
cashpack_orig_cflags_$1="$cashpack_orig_cflags_$1 m4_normalize([$2])"
CFLAGS="$cashpack_chkd_cflags_$1 ${cashpack_orig_cflags_$1#orig:}"
}
])

# CASHPACK_CHECK_LIB(LIBRARY, FUNCTION[, SEARCH-LIBS])
# ---------------------------------------------------
AC_DEFUN([CASHPACK_CHECK_LIB], [
cashpack_save_LIBS="$LIBS"
LIBS=""
AC_SEARCH_LIBS([$2], [$1 $3], [], [AC_MSG_ERROR([Could not find $2 support])])
AC_SUBST(m4_toupper([$1])_LIBS, [$LIBS])
LIBS="$cashpack_save_LIBS"
])

# CASHPACK_CHECK_PROG(VARIABLE, PROGS, DESC)
# -----------------------------------------
AC_DEFUN([CASHPACK_CHECK_PROG], [
AC_ARG_VAR([$1], [$3])
{
AC_CHECK_PROGS([$1], [$2], [no])
test "$[$1]" = no &&
AC_MSG_ERROR([Could not find program $2])
}
])

# CASHPACK_COND_PROG(VARIABLE, PROGS, DESC)
# -----------------------------------------
AC_DEFUN([CASHPACK_COND_PROG], [
AC_ARG_VAR([$1], [$3])
AC_CHECK_PROGS([$1], [$2], [no])
AM_CONDITIONAL([HAVE_$1], [test "$$1" != no])
])

# CASHPACK_COND_MODULE(VARIABLE, MODULE)
# --------------------------------------
AC_DEFUN([CASHPACK_COND_MODULE], [
PKG_CHECK_MODULES([$1], [$2], [$1=yes], [$1=no])
AC_SUBST([$1])
AM_CONDITIONAL([HAVE_$1], [test "$$1" = yes])
])

# CASHPACK_GOLANG_PREREQ(VERSION)
# -------------------------------
AC_DEFUN([CASHPACK_GOLANG_PREREQ], [

	AC_MSG_CHECKING([for golang >= $1])

	GOROOT="$($GO env GOROOT)"

	[GOLANG_VERSION="$(
		$GO version 2>/dev/null |
		tr ' ' '\n' |
		grep '^go[1-9]' |
		$SED -e s/go//
	)"]

	AC_SUBST([GOROOT])
	AC_SUBST([GOLANG_VERSION])

	AS_VERSION_COMPARE([$GOLANG_VERSION], [$1],
		[cashpack_golang_prereq=no],
		[cashpack_golang_prereq=yes],
		[cashpack_golang_prereq=yes])

	AC_MSG_RESULT([$cashpack_golang_prereq])
	AM_CONDITIONAL([HAVE_GOLANG], [test "$cashpack_golang_prereq" = yes])

	dnl Define an automake silent execution for go
	[am__v_GO_0='@echo "  GO      " $''@;']
	[am__v_GO_1='']
	[am__v_GO_='$(am__v_GO_$(AM_DEFAULT_VERBOSITY))']
	[AM_V_GO='$(am__v_GO_$(V))']
	AC_SUBST([am__v_GO_0])
	AC_SUBST([am__v_GO_1])
	AC_SUBST([am__v_GO_])
	AC_SUBST([AM_V_GO])

])
