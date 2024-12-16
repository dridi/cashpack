/*-
 * Copyright (c) 2016-2020 Dridi Boukelmoune
 * All rights reserved.
 *
 * Author: Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "hpack.h"
#include "hpack_assert.h"
#include "hpack_priv.h"

#include "tst.h"

struct dec_priv {
	struct hpack	*hp;
	hpack_event_f	*cb;
	void		*buf;
	size_t		len;
	unsigned	skp;
	unsigned	mon;
	ssize_t		off;
};

static void
print_nothing(enum hpack_event_e evt, const char *buf, size_t len, void *priv)
{

	assert(priv != NULL);

#ifdef NDEBUG
	(void)priv;
#endif

	(void)evt;
	(void)buf;
	(void)len;
}

static void
print_headers(enum hpack_event_e evt, const char *buf, size_t len, void *priv)
{
	const struct dec_ctx *ctx;
	struct dec_priv *dp;

	assert(priv != NULL);
	ctx = priv;
	dp = ctx->priv;

	switch (evt) {
	case HPACK_EVT_FIELD:
		if (dp->mon && dp->off >= 0)
			OUT(" (+0x%zx)", dp->off);
		OUT("\n");
		dp->off = ctx->acc_len; /* XXX: missing field offset */
		break;
	case HPACK_EVT_VALUE:
		OUT(": ");
		fallthrough;
	case HPACK_EVT_NAME:
		WRT(buf, len);
		fallthrough;
	default:
		break;
	}
}

static int
decode_block(struct dec_ctx *ctx, const void *blk, size_t len, unsigned cut)
{
	struct dec_priv *dp;
	struct hpack_decoding dec;
	int retval;

	dp = ctx->priv;
	dec.blk = blk;
	dec.blk_len = len;
	dec.buf = dp->buf;
	dec.buf_len = dp->len;
	dec.cb = dp->cb;
	dec.priv = ctx;
	dec.cut = cut;

	retval = hpack_decode(dp->hp, &dec);

	if (retval == HPACK_RES_OK) {
		assert(!cut);
		if (dp->mon && dp->off >= 0)
			OUT(" (+0x%zx)", dp->off);
	}

	if (retval == HPACK_RES_BLK) {
		assert(cut);
		if (!dp->skp)
			retval = HPACK_RES_OK;
	}

	return (retval);
}

static int
skip_block(struct dec_ctx *ctx, const void *blk, size_t len, unsigned cut)
{
	struct dec_priv *dp;
	int retval;

	dp = ctx->priv;
	dp->skp = 1;
	retval = decode_block(ctx, blk, len, cut);
	dp->skp = 0;

	if (retval == HPACK_RES_BLK) {
		assert(cut);
		retval = HPACK_RES_OK;
	}
	else {
		assert(retval == HPACK_RES_SKP);
		OUT("<too big>");
		assert(!cut);
		retval = hpack_skip(dp->hp);
	}

	return (retval);
}

static int
resize_table(struct dec_ctx *ctx, const void *buf, size_t len, unsigned cut)
{
	struct dec_priv *dp;

	(void)buf;
	(void)cut;
	dp = ctx->priv;
	assert(dp->skp == 0);
	return (hpack_resize(&dp->hp, len));
}

int
main(int argc, char **argv)
{
	enum hpack_result_e res, exp;
	hpack_event_f *cb;
	struct dec_ctx ctx;
	struct dec_priv priv;
	struct stat st;
	char buf[4096];
	void *blk;
	int fd, retval, tbl_sz;

	TST_signal();

	priv.buf = buf;
	priv.len = sizeof buf;
	priv.skp = 0;
	priv.mon = 0;
	priv.off = -1;

	ctx.dec = decode_block;
	ctx.skp = skip_block;
	ctx.rsz = resize_table;
	ctx.priv = &priv;
	ctx.spec = "";
	ctx.acc_len = 0;
	tbl_sz = 4096; /* RFC 7540 Section 6.5.2 */
	exp = HPACK_RES_OK;
	cb = print_headers;

	/* ignore the command name */
	argc--;
	argv++;

	/* handle options */
	if (argc > 0 && !strcmp("--monitor", *argv)) {
		priv.mon = 1;
		argc -= 1;
		argv += 1;
	}

	if (argc > 0 && !strcmp("--buffer-size", *argv)) {
		assert(argc > 2);
		priv.len = atoi(argv[1]);
		assert(priv.len > 0);
		assert(priv.len < sizeof buf);
		argc -= 2;
		argv += 2;
	}

	if (argc > 0 && !strcmp("--decoding-spec", *argv)) {
		assert(argc > 2);
		ctx.spec = argv[1];
		argc -= 2;
		argv += 2;
	}

	if (argc > 0 && !strcmp("--expect-error", *argv)) {
		assert(argc > 2);
		exp = TST_translate_error(argv[1]);
		assert(exp < 0);
		cb = print_nothing;
		argc -= 2;
		argv += 2;
	}

	if (argc > 0 && !strcmp("--table-size", *argv)) {
		assert(argc > 2);
		tbl_sz = atoi(argv[1]);
		assert(tbl_sz > 0);
		argc -= 2;
		argv += 2;
	}

	/* exactly one file name is expected */
	if (argc != 1) {
		fprintf(stderr,
		    "Usage: hdecode [--monitor] [--expect-error <ERR>] "
		    "[--decoding-spec <spec>,[...]] [--table-size <size>] "
		    "[--buffer-size <size>] <dump file>\n\n"
		    "The file contains a dump of HPACK octets.\n\n"
		    "Spec format: <letter><size>\n"
		    "  a - abort the decoding process\n"
		    "  d - decode a block of <size> bytes from the dump\n"
		    "  p - decode a partial block of <size> bytes\n"
		    "  r - resize the dynamic table to <size> bytes\n"
		    "  s - try to decode <size> bytes and skip the rest\n"
		    "  S - the same as 's' but for partial blocks\n"
		    "  The last empty spec decodes the rest of the dump\n"
		    "Default table size: 4096\n"
		    "Possible errors:\n");

#define HPR_ERRORS_ONLY
#define HPR(val, cod, txt, rst) fprintf(stderr, "  %s: %s\n", #val, txt);
#include "tbl/hpack_tbl.h"
#undef HPR
#undef HPR_ERRORS_ONLY

		return (EXIT_FAILURE);
	}

	fd = open(*argv, O_RDONLY);
	assert(fd > STDERR_FILENO);

	retval = fstat(fd, &st);
	assert(retval == 0);
	ctx.blk_len = st.st_size;

#ifdef NDEBUG
	(void)retval;
#endif

	blk = malloc(st.st_size);
	assert(blk != NULL);

	retval = read(fd, blk, st.st_size);
	assert(retval == (int)st.st_size);

	retval = close(fd);
	assert(retval == 0);

	ctx.blk = blk;

	if (priv.mon)
		hp = hpack_monitor(tbl_sz, -1, hpack_default_alloc);
	else
		hp = hpack_decoder(tbl_sz, -1, hpack_default_alloc);
	assert(hp != NULL);

	priv.hp = hp;
	priv.cb = cb;
	res = TST_decode(&ctx);

	OUT("\n\n");
	TST_print_table();

	hpack_free(&hp);
	free(blk);

	if (res != exp)
		ERR("hpack error: expected '%s' (%d) got '%s' (%d)",
		    hpack_strerror(exp), exp, hpack_strerror(res), res);

	return (res != exp);
}
