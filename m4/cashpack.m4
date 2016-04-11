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

	AC_CHECK_LIB(
		[asan],
		[__asan_address_is_poisoned], [
			LIBS="$ac_check_lib_save_LIBS"
			CFLAGS="$CFLAGS -fsanitize=address"
		],
		[AC_MSG_FAILURE([Missing libasan for address sanitizer])])

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

	AC_CHECK_LIB(
		[ubsan],
		[__ubsan_handle_add_overflow], [
			LIBS="$ac_check_lib_save_LIBS"
			CFLAGS="$CFLAGS -fsanitize=undefined"
		],
		[AC_MSG_FAILURE([Missing libubsan for undefined sanitizer])])

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
