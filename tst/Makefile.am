# Copyright (c) 2016 Dridi Boukelmoune
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

AM_CFLAGS = -Wall -Wextra -Werror

AM_CPPFLAGS = \
	-I $(top_srcdir)/inc \
	-I $(top_builddir)/inc

noinst_PROGRAMS = \
	hpack_arg \
	hdecode \
	hencode

hpack_arg_LDADD = $(top_builddir)/lib/libhpack.la
hpack_arg_SOURCES = hpack_arg.c

hdecode_LDADD = $(top_builddir)/lib/libhpack.la
hdecode_SOURCES = \
	tst.h \
	tst.c \
	hdecode.c

hencode_LDADD = $(top_builddir)/lib/libhpack.la
hencode_SOURCES = \
	tst.h \
	tst.c \
	hencode.c

if HAVE_NGHTTP2
noinst_PROGRAMS += ngdecode
ngdecode_CFLAGS = $(NGHTTP2_CFLAGS)
ngdecode_LDFLAGS = $(NGHTTP2_LIBS)
ngdecode_LDADD = $(top_builddir)/lib/libhpack.la
ngdecode_SOURCES = \
	tst.h \
	tst.c \
	ngdecode.c
endif

check: $(noinst_PROGRAMS)

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
	rfc7540_8_1_2

COV_TESTS = \
	hpack_arg \
	hpack_cov \
	hpack_dec \
	hpack_enc \
	hpack_huf \
	hpack_tbl

REG_TESTS = \
	afl_fuzz

AM_TESTS_ENVIRONMENT = \
	MEMCHECK="$(MEMCHECK)" \
	NGHTTP2="$(NGHTTP2)"

TESTS = $(RFC_TESTS) $(COV_TESTS) $(REG_TESTS)

EXTRA_DIST = \
	$(TESTS) \
	hexcheck \
	bincheck \
	hex_decode \
	hex_encode \
	common.sh
