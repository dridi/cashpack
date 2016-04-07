AM_CFLAGS = -Wall -Werror -Wextra
AM_CPPFLAGS = -I$(top_srcdir)/inc

noinst_PROGRAMS = hphidx hphtbl

hphidx_SOURCES = hphidx.c
hphtbl_SOURCES = hphtbl.c

BUILT_SOURCES = \
	hpack_huf_idx.h \
	hpack_huf_tbl.h

hpack_huf_idx.h: hphidx
	$(AM_V_GEN) ./hphidx >hpack_huf_idx.h

hpack_huf_tbl.h: hphtbl
	$(AM_V_GEN) ./hphtbl >hpack_huf_tbl.h

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
	$(BUILT_SOURCES) \
	gen.h
