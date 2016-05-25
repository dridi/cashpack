/*-
 * Copyright (c) 2016 Dridi Boukelmoune
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
#include <ctype.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hpack.h"
#include "hpack_assert.h"
#include "hpack_priv.h"

#include "tst.h"

/**********************************************************************
 * Dynamic table
 */

struct dyn_ctx {
	size_t	cnt;
	size_t	len;
	size_t	sz;
};

static void
tst_print_cb(void *priv, enum hpack_event_e evt, const char *buf, size_t len)
{
	struct dyn_ctx *ctx;
	char str[sizeof "\n[IDX] (s = LEN) "];
	int l;

	assert(priv != NULL);
	ctx = priv;
	if (ctx->cnt == 0)
		OUT("\n");

	switch (evt) {
	case HPACK_EVT_FIELD:
		assert(buf == NULL);
		assert(len > 0);
		ctx->cnt++;
		ctx->len += len;
		l = snprintf(str, sizeof str, "\n[%3zu] (s = %3zu) ",
		    ctx->cnt, len);
		assert(l + 1 == sizeof  str);
		WRT(str, l);
		break;
	case HPACK_EVT_VALUE:
		OUT(": ");
		/* fall through */
	case HPACK_EVT_NAME:
		assert(buf != NULL);
		WRT(buf, len);
		break;
	default:
		WRONG("Unexpected event");
	}
}

void
TST_print_table(struct hpack *hp)
{
	struct dyn_ctx ctx;
	char buf[8];

	ctx.cnt = 0;
	ctx.len = 0;

	OUT("Dynamic Table (after decoding):");

	(void)hpack_foreach(hp, tst_print_cb, &ctx);

	if (ctx.cnt == 0) {
		assert(ctx.len == 0);
		OUT(" empty.\n");
	}
	else {
		assert(ctx.len > 0);
		ctx.sz = snprintf(buf, sizeof buf, "%3zu\n", ctx.len);
		OUT("\n      Table size: ");
		WRT(buf, ctx.sz);
	}
}

/**********************************************************************
 * Decoding process
 */

int
TST_decode(struct dec_ctx *ctx)
{
	tst_decode_f *cb;
	size_t len;
	unsigned cut;
	int res;

	assert(ctx->len > 0);

	OUT("Decoded header list:\n");
	res = 0;

	do {
		assert(res == 0);
		cut = 0;

		switch (*ctx->spec) {
		case '\0':
			cb = ctx->dec;
			len = ctx->len;
			break;
		case 'p':
			cut = 1;
			/* fall through */
		case 'd':
			ctx->spec++;

			cb = ctx->dec;
			len = atoi(ctx->spec);
			assert(len <= ctx->len);
			break;
		case 'r':
			ctx->spec++;

			cb = ctx->rsz;
			len = atoi(ctx->spec);
			break;
		default:
			WRONG("Invalid spec");
		}

		if (*ctx->spec != '\0') {
			ctx->spec = strchr(ctx->spec, ',');
			assert(ctx->spec != NULL);
			ctx->spec++;
		}

		res = cb(ctx->priv, ctx->buf, len, cut);

		if (cb == ctx->dec) {
			ctx->buf += len;
			ctx->len -= len;
		}
	} while (ctx->len > 0);

	return (res);
}

enum hpack_result_e
TST_translate_error(const char *str)
{
#define HPR_ERRORS_ONLY
#define HPR(val, cod, txt, rst)			\
		if (!strcmp(str, #val))	\
			return (HPACK_RES_##val);
#include "tbl/hpack_tbl.h"
#undef HPR
#undef HPR_ERRORS_ONLY
	WRONG("Unknown error");
	return (-1);
}

/**********************************************************************
 * Signal handling
 */

extern struct hpack *hp;

static void
tst_hexdump(void *ptr, ssize_t len, const char *pfx)
{
	uint8_t *buf;
	ssize_t pos;
	int i;

	buf = ptr;
	pos = 0;

	while (len > 0) {
		fprintf(stderr, "%s%06zx: ", pfx, pos);
		for (i = 0; i < 16; i++)
			if (i < len)
				fprintf(stderr, "%02x ", buf[i]);
			else
				fprintf(stderr, "   ");
		fprintf(stderr, "| ");
		for (i = 0; i < 16; i++)
			if (i < len)
				fprintf(stderr, "%c",
				    isprint(buf[i]) ? buf[i] : '.');
		fprintf(stderr, "\n");
		len -= 16;
		buf += 16;
		pos += 16;
	}
}

static void
tst_dump(int signo)
{
	uint8_t *ptr;

	(void)signo;

	if (hp == NULL)
		return;

	fprintf(stderr, "*hp = %p {\n", hp);
	fprintf(stderr, "\t.magic = %08x\n", hp->magic);
	fprintf(stderr, "\t.alloc = {\n");
	fprintf(stderr, "\t\t.malloc = %p\n", hp->alloc.malloc);
	fprintf(stderr, "\t\t.realloc = %p\n", hp->alloc.realloc);
	fprintf(stderr, "\t\t.free = %p\n", hp->alloc.free);
	fprintf(stderr, "\t}\n");
	fprintf(stderr, "\t.sz = {\n");
	fprintf(stderr, "\t\t.mem = %zu\n", hp->sz.mem);
	fprintf(stderr, "\t\t.max = %zu\n", hp->sz.max);
	fprintf(stderr, "\t\t.lim = %zu\n", hp->sz.lim);
	fprintf(stderr, "\t\t.cap = %zd\n", hp->sz.cap);
	fprintf(stderr, "\t\t.len = %zu\n", hp->sz.len);
	fprintf(stderr, "\t\t.nxt = %zd\n", hp->sz.nxt);
	fprintf(stderr, "\t\t.min = %zd\n", hp->sz.min);
	fprintf(stderr, "\t}\n");
	fprintf(stderr, "\t.state = {\n");
	// XXX: do when bored
	fprintf(stderr, "\t}\n");
	fprintf(stderr, "\t.ctx = {\n");
	// XXX: do when bored
	fprintf(stderr, "\t}\n");
	fprintf(stderr, "\t.cnt = %zu\n", hp->cnt);
	fprintf(stderr, "\t.off = %zd\n", hp->off);

	ptr = (uint8_t *)hp->tbl + hp->off;
	fprintf(stderr, "\t.tbl = %p <<EOF\n", ptr);
	tst_hexdump(ptr, hp->sz.len, "\t");
	fprintf(stderr, "\tEOF\n");
	fprintf(stderr, "}\n");
}

void
TST_signal()
{

	if (signal(SIGABRT, tst_dump) == SIG_ERR) {
		ERR("%s", "Failed to install signal handler");
		exit(EXIT_FAILURE);
	}
}
