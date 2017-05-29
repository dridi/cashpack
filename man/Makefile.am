# Copyright (c) 2016-2017 Dridi Boukelmoune
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

CFLAGS += $(EXAMPLE_CFLAGS)

AM_CPPFLAGS = \
	-I$(top_srcdir)/inc \
	-I$(top_builddir)/inc

BUILD_MAN_PAGE = $(RST2MAN) --strict --strip-comments
BUILD_MAN_LINK = printf ".so man3/%s\n"

hpack_alloc_links = \
	hpack_decoder.3 \
	hpack_encoder.3 \
	hpack_free.3 \
	hpack_resize.3 \
	hpack_limit.3 \
	hpack_trim.3

hpack_decode_links = \
	hpack_decode_fields.3

hpack_error_links = \
	hpack_strerror.3 \
	hpack_dump.3

hpack_index_links = \
	hpack_static.3 \
	hpack_dynamic.3 \
	hpack_tables.3 \
	hpack_entry.3

if HAVE_HEXDUMP
dist_man_MANS = \
	cashpack.3 \
	hpack_alloc.3 \
	hpack_decode.3 \
	hpack_encode.3 \
	hpack_index.3 \
	hpack_error.3 \
	$(hpack_alloc_links) \
	$(hpack_decode_links) \
	$(hpack_error_links) \
	$(hpack_index_links)
endif

# code examples

examplesdir = $(pkgdatadir)/examples

examples_DATA = \
	cashdumb.c \
	cashdump.c \
	hpack_dump.c

if DOCS

# doc generators

noinst_PROGRAMS = \
	cashdumb \
	cashdump \
	hpack_dump \
	hpe2rst.gen \
	hpf2rst.gen \
	hpr2rst.gen

cashdumb_LDADD = $(top_builddir)/lib/libhpack.la
cashdump_LDADD = $(top_builddir)/lib/libhpack.la
hpack_dump_LDADD = $(top_builddir)/lib/libhpack.la

.gen.rst:
	$(AM_V_GEN) ./$< >$@

.c.src:
	@echo '$$ cat' $$(basename '$<') >$@
	$(AM_V_GEN) $(SED) '1,28d' <$< >>$@

cashdumb.txt: requests.txt
	@echo '$$ cat requests.txt' >$@
	$(AM_V_GEN) cat $(srcdir)/requests.txt >$@

cashdumb.out: cashdumb requests.txt
	@echo '$$ gcc -lhpack -o cashdumb cashdumb.c' >$@
	@echo '$$ ./cashdumb <requests.txt >/dev/null' >>$@
	$(AM_V_GEN) ./cashdumb <$(srcdir)/requests.txt >/dev/null 2>>$@

cashdump.hex: frames.hex
	@echo '$$ hexdump -C frames.bin' >$@
	$(AM_V_GEN) $(top_srcdir)/tst/hex_decode <$(srcdir)/frames.hex | \
		hexdump -C >>$@

cashdump.out: cashdump frames.hex
	@echo '$$ gcc -lhpack -o cashdump cashdump.c' >$@
	@echo '$$ ./cashdump <frames.bin' >>$@
	$(AM_V_GEN) $(top_srcdir)/tst/hex_decode <$(srcdir)/frames.hex | \
		./cashdump >>$@

# man pages

SUFFIXES = .3.rst .3

.3.rst.3:
	$(AM_V_GEN) $(BUILD_MAN_PAGE) $< $@

cashpack.3: hpe2rst.rst cashdump.src cashdump.hex cashdump.out

hpack_encode.3: hpf2rst.rst cashdumb.src cashdumb.txt cashdumb.out

hpack_error.3: hpr2rst.rst hpack_dump.src

# man links

$(hpack_alloc_links):
	$(AM_V_GEN) $(BUILD_MAN_LINK) hpack_alloc.3 >$@

$(hpack_decode_links):
	$(AM_V_GEN) $(BUILD_MAN_LINK) hpack_decode.3 >$@

$(hpack_error_links):
	$(AM_V_GEN) $(BUILD_MAN_LINK) hpack_error.3 >$@

$(hpack_index_links):
	$(AM_V_GEN) $(BUILD_MAN_LINK) hpack_index.3 >$@

hpack_clean_field.3:
	$(AM_V_GEN) $(BUILD_MAN_LINK) hpack_encode.3 >$@

# cleanup

clean-local:
	@test "$(abs_srcdir)" = "$(abs_builddir)" || \
		rm -f $(builddir)/*.rst

DISTCLEANFILES = \
	$(dist_man_MANS)

CLEANFILES = \
	hpe2rst.rst \
	hpf2rst.rst \
	hpr2rst.rst \
	cashdumb.txt \
	cashdumb.out \
	cashdump.hex \
	cashdump.out \
	cashdumb.src \
	cashdump.src

endif

EXTRA_DIST = \
	cashpack.3.rst \
	hpack_alloc.3.rst \
	hpack_decode.3.rst \
	hpack_encode.3.rst \
	hpack_index.3.rst \
	hpack_error.3.rst \
	frames.hex \
	requests.txt
