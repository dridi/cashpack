BUILD_MAN = rst2man --strict --strip-comments

dist_man_MANS = \
	cashpack.3 \
	hpack_alloc.3 \
	hpack_decode.3 \
	hpack_decoder.3 \
	hpack_encoder.3 \
	hpack_foreach.3 \
	hpack_free.3

CLEANFILES = $(dist_man_MANS)

EXTRA_DIST = \
	cashpack.3.rst \
	hpack_alloc.3.rst \
	hpack_decode.3.rst \
	hpack_foreach.3.rst

cashpack.3: cashpack.3.rst
	$(AM_V_GEN) $(BUILD_MAN) $(srcdir)/cashpack.3.rst cashpack.3

hpack_alloc.3: hpack_alloc.3.rst
	$(AM_V_GEN) $(BUILD_MAN) $(srcdir)/hpack_alloc.3.rst hpack_alloc.3

hpack_decoder.3:
	$(AM_V_GEN) echo ".so man3/hpack_alloc.3" >hpack_decoder.3

hpack_encoder.3:
	$(AM_V_GEN) echo ".so man3/hpack_alloc.3" >hpack_encoder.3

hpack_free.3:
	$(AM_V_GEN) echo ".so man3/hpack_alloc.3" >hpack_free.3

hpack_decode.3: hpack_decode.3.rst
	$(AM_V_GEN) $(BUILD_MAN) $(srcdir)/hpack_decode.3.rst hpack_decode.3

hpack_foreach.3: hpack_foreach.3.rst
	$(AM_V_GEN) $(BUILD_MAN) $(srcdir)/hpack_foreach.3.rst hpack_foreach.3
