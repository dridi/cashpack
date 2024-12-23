# License: BSD-2-Clause
# (c) 2016-2019 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>

AM_CPPFLAGS = \
	-I$(top_srcdir)/inc \
	-I$(top_builddir)/inc \
	-I$(top_builddir)/gen

noinst_LTLIBRARIES = libhpi.la

libhpi_la_SOURCES = \
	hpack_ctx.c \
	hpack_enc.c \
	hpack_int.c

lib_LTLIBRARIES = libhpack.la

if HAVE_LD_VERSION_SCRIPT
libhpack_la_LDFLAGS = -Wl,--version-script=$(srcdir)/$(PACKAGE).map
else
libhpack_la_LDFLAGS = -export-symbols-regex '^hpack_'
endif

libhpack_la_LDFLAGS += -version-info 0:0:0

libhpack_la_LIBADD = libhpi.la

libhpack_la_DEPENDENCIES = \
	$(libhpack_la_LIBADD) \
	$(PACKAGE).map

libhpack_la_SOURCES = \
	hpack.c \
	hpack_dec.c \
	hpack_huf.c \
	hpack_tbl.c \
	hpack_val.c \
	$(top_builddir)/inc/hpack.h \
	$(top_builddir)/inc/tbl/hpack_pseudo_headers.h \
	$(top_builddir)/inc/tbl/hpack_static.h \
	$(top_builddir)/inc/tbl/hpack_tbl.h \
	$(top_builddir)/gen/hpack_huf_dec.h \
	$(top_builddir)/gen/hpack_huf_enc.h \
	$(top_builddir)/gen/hpack_static_hdr.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(PACKAGE).pc

noinst_PROGRAMS = hpiencode

hpiencode_LDADD = libhpi.la

hpiencode_DEPENDENCIES = $(hpiencode_LDADD)

EXTRA_DIST = \
	$(PACKAGE).pc.in \
	$(PACKAGE).map
