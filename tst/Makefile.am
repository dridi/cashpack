# License: BSD-2-Clause
# (c) 2016-2024 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>

CFLAGS += $(TEST_CFLAGS)

AM_CPPFLAGS = \
	-I$(top_srcdir)/inc \
	-I$(top_builddir)/inc

# Programs used by the test suite

check_PROGRAMS = \
	hpack_arg \
	hpack_dep \
	hpack_mbm \
	hdecode \
	fdecode \
	hencode

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
GOPATH = $(abs_builddir)/go

check_PROGRAMS += godecode

go.sum: go.mod
	$(AM_V_GO)GOPATH=$(GOPATH) $(GO) mod tidy
	@touch $@

go.mod:
	test -f godecode.go || ln -s $(abs_srcdir)/godecode.go godecode.go
	$(AM_V_GO)GOPATH=$(GOPATH) $(GO) mod init cashpack/tst

godecode_SOURCES = godecode.go
godecode_DEPENDENCIES = go.mod go.sum
godecode_LINK = GOPATH=$(GOPATH) $(GO) build -o $@ $(godecode_SOURCES)

clean-golang:
	GOPATH=$(GOPATH) $(GO) clean -modcache
	rm -rf $(GOPATH) $(godecode_DEPENDENCIES)

clean-local: clean-golang
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
	hpack_mon \
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

EXTRA_DIST = \
	$(TESTS) \
	hexcheck \
	bincheck \
	hex_decode \
	hex_encode \
	common.sh \
	fuzz_build_corpus
