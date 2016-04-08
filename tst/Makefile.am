AM_CFLAGS = -Wall -Wextra -Werror

AM_CPPFLAGS = \
	-I $(top_srcdir)/inc \
	-I $(top_builddir)/inc

noinst_PROGRAMS = hdecode
hdecode_SOURCES = hdecode.c
hdecode_LDADD = $(top_builddir)/lib/libhpack.la

check: $(noinst_PROGRAMS)

RFC_TESTS = \
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
	rfc7541_c_6_3

COV_TESTS = \
	hpack_cov \
	hpack_huf \
	hpack_int \
	hpack_tbl

REG_TESTS = \
	afl_fuzz

TESTS = $(RFC_TESTS) $(COV_TESTS) $(REG_TESTS)

EXTRA_DIST = \
	$(TESTS) \
	hexcheck \
	bincheck \
	hex_decode \
	hex_encode \
	common.sh
