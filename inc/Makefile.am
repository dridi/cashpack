# License: BSD-2-Clause
# (c) 2016-2017 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>

include_HEADERS = hpack.h
BUILT_SOURCES = hpack.h
DISTCLEANFILES = hpack.h

hpack.h: hpack.h.in format_header.sh tbl/hpack_tbl.h uncrustify.cfg
	$(AM_V_GEN) $(srcdir)/format_header.sh $(srcdir)/hpack.h.in $@

EXTRA_DIST = \
	format_header.sh \
	hpack.h.in \
	hpack.h \
	hpack_assert.h \
	hpack_priv.h \
	tbl/hpack_tbl.h \
	tbl/hpack_huffman.h \
	tbl/hpack_static.h \
	uncrustify.cfg
