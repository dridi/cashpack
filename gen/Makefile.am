# License: BSD-2-Clause
# (c) 2016-2017 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>

AM_CPPFLAGS = -I$(top_srcdir)/inc

noinst_PROGRAMS = \
	hpack_huf_dec.gen \
	hpack_huf_enc.gen \
	hpack_static_hdr.gen

BUILT_SOURCES = \
	hpack_huf_dec.h \
	hpack_huf_enc.h \
	hpack_static_hdr.h

.gen.h:
	@rm -f .$@
	$(AM_V_GEN) ./$< >.$@
	@mv .$@ $@

EXTRA_DIST = $(BUILT_SOURCES) gen.h
DISTCLEANFILES = $(BUILT_SOURCES)
