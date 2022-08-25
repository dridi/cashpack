# Copyright (c) 2016-2020 Dridi Boukelmoune
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

CFLAGS += $(TEST_CFLAGS)

AM_CPPFLAGS = \
	-I$(top_srcdir)/inc \
	-I$(top_builddir)/inc

# Programs used by the test suite

check_PROGRAMS = \
	hpack_arg \
	hpack_mbm \
	hdecode \
	fdecode \
	hencode

nodist_noinst_SCRIPTS = cov.sh

SUFFIXES = .sh.in .sh

.sh.in.sh:
	$(AM_V_GEN) $(SED) \
		-e 's:[@]builddir[@]:$(abs_top_builddir):' \
		-e 's:[@]GCOV[@]:$(GCOV):' \
		-e 's:[@]JQ[@]:$(JQ):' \
		-e 's:[@]NC[@]:$(NC):' \
		<'$<' >$@
	chmod +x $@

all-local: $(check_PROGRAMS)

hpack_arg_LDADD = $(top_builddir)/lib/libhpack.la
hpack_arg_SOURCES = \
	hpack_arg.c \
	$(top_srcdir)/inc/dbg.h

hpack_mbm_LDADD = $(top_builddir)/lib/libhpack.la

hdecode_LDADD = $(top_builddir)/lib/libhpack.la
hdecode_SOURCES = \
	tst.h \
	tst.c \
	hdecode.c

fdecode_LDADD = $(top_builddir)/lib/libhpack.la
fdecode_SOURCES = \
	tst.h \
	tst.c \
	fdecode.c

hencode_LDADD = $(top_builddir)/lib/libhpack.la
hencode_SOURCES = \
	tst.h \
	tst.c \
	hencode.c

if HAVE_NGHTTP2
check_PROGRAMS += ngdecode
ngdecode_CFLAGS = $(NGHTTP2_CFLAGS)
ngdecode_LDFLAGS = $(NGHTTP2_LIBS)
ngdecode_LDADD = $(top_builddir)/lib/libhpack.la
ngdecode_SOURCES = \
	tst.h \
	tst.c \
	ngdecode.c
endif

if HAVE_GOLANG
check_PROGRAMS += godecode

.go.o:
	$(AM_V_GO)$(GO) tool compile -o $@ $<

godecode_SOURCES = godecode.go
godecode_LINK = $(GO) tool link -o $@
endif

# Test droppings cleanup

clean-failed:
	rm -rf $(builddir)/failed-cashpack.*/

clean-memcheck:
	find $(builddir) -name 'memcheck-*.log' -delete

clean-local: clean-failed clean-memcheck

check-am: clean-failed clean-memcheck $(PROGRAMS)

# Test suite

RFC_TESTS = \
	rfc7541_2_3_2 \
	rfc7541_2_3_3 \
	rfc7541_4_1 \
	rfc7541_4_2 \
	rfc7541_4_3 \
	rfc7541_4_4 \
	rfc7541_5_1 \
	rfc7541_5_2 \
	rfc7541_6_1 \
	rfc7541_6_3 \
	rfc7541_c_2_1 \
	rfc7541_c_2_2 \
	rfc7541_c_2_3 \
	rfc7541_c_2_4 \
	rfc7541_c_3_1 \
	rfc7541_c_3_2 \
	rfc7541_c_3_3 \
	rfc7541_c_4_1 \
	rfc7541_c_4_2 \
	rfc7541_c_4_3 \
	rfc7541_c_5_1 \
	rfc7541_c_5_2 \
	rfc7541_c_5_3 \
	rfc7541_c_6_1 \
	rfc7541_c_6_2 \
	rfc7541_c_6_3 \
	rfc7230_3_2 \
	rfc7540_4_3 \
	rfc7540_8_1_2 \
	rfc8441_3

COV_TESTS = \
	hpack_arg \
	hpack_cov \
	hpack_dec \
	hpack_enc \
	hpack_huf \
	hpack_mbm \
	hpack_skp \
	hpack_tbl

REG_TESTS = \
	hpack_fuzz

AM_TESTS_ENVIRONMENT = \
	KEEP_TMP="$(KEEP_TMP)" \
	MEMCHECK="$(MEMCHECK)" \
	VALGRIND="$(VALGRIND)"

TESTS = $(RFC_TESTS) $(COV_TESTS) $(REG_TESTS)

if FUZZ
check_PROGRAMS += fuzzdecode

fuzzdecode_CFLAGS = -fsanitize=fuzzer
fuzzdecode_LDADD = $(top_builddir)/lib/libhpack.la

fuzz-seeds: $(srcdir)/fuzz_build_corpus $(TESTS)
	$(AM_V_GEN) $(srcdir)/fuzz_build_corpus $(srcdir) $@

fuzz-local: fuzzdecode fuzz-seeds
	mkdir -p fuzz-corpus
	./fuzzdecode -max_len=4096 fuzz-corpus fuzz-seeds $(FUZZ_OPTS)
endif

CLEANFILES = $(nodist_noinst_SCRIPTS)
EXTRA_DIST = \
	$(TESTS) \
	hexcheck \
	bincheck \
	hex_decode \
	hex_encode \
	common.sh \
	cov.sh.in \
	fuzz_build_corpus
