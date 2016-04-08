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

# CASHPACK_CHECK_UNCRUSTIFY
# -------------------------
AC_DEFUN([CASHPACK_CHECK_UNCRUSTIFY], [

	UNCRUSTIFY_OPTS="-c '\$(srcdir)/uncrustify.cfg' -q -l C --no-backup"

	AC_CHECK_PROGS(UNCRUSTIFY, [uncrustify], [true])
	test "$UNCRUSTIFY" = true && UNCRUSTIFY_OPTS=

	AC_SUBST([UNCRUSTIFY])
	AC_SUBST([UNCRUSTIFY_OPTS])

])
