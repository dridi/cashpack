include_HEADERS = hpack.h
BUILT_SOURCES = hpack.h
CLEANFILES = hpack.h

hpack.h: hpack.h.in
	$(AM_V_GEN) cpp -o hpack.h $(srcdir)/hpack.h.in
	@$(SED) -i "/^#/d" hpack.h
	@$(UNCRUSTIFY) $(UNCRUSTIFY_OPTS) hpack.h

EXTRA_DIST = \
	hpack.h.in \
	hpack.h \
	hpack_assert.h \
	hpack_priv.h \
	tbl/hpack_tbl.h \
	tbl/hpack_huffman.h \
	tbl/hpack_static.h \
	uncrustify.cfg
