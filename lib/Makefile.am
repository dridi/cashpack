AM_CFLAGS = -Wall -Werror -Wextra

AM_CPPFLAGS = \
	-I $(top_srcdir)/inc \
	-I $(top_builddir)/inc \
	-I $(top_builddir)/gen

AM_LDFLAGS = \
	$(AM_LT_LDFLAGS) \
		-export-symbols-regex '^hpack_'

lib_LTLIBRARIES = libhpack.la

libhpack_la_SOURCES = \
	hpack.c \
	hpack_huf.c \
	hpack_int.c \
	hpack_tbl.c \
	hpack_val.c \
	$(top_builddir)/inc/hpack.h \
	$(top_builddir)/gen/hpack_huf_idx.h \
	$(top_builddir)/gen/hpack_huf_tbl.h
