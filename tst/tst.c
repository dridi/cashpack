/*-
 * Copyright (c) 2016-2017 Dridi Boukelmoune
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
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
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

struct hpack *hp;

/**********************************************************************
 * Dynamic table
 */

struct dyn_ctx {
	size_t	cnt;
	size_t	len;
	size_t	sz;
};

static void
tst_print_cb(enum hpack_event_e evt, const char *buf, size_t len, void *priv)
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
TST_print_table(void)
{
	struct dyn_ctx ctx;
	char buf[8];

	ctx.cnt = 0;
	ctx.len = 0;

	OUT("Dynamic Table (after decoding):");

	(void)hpack_dynamic(hp, tst_print_cb, &ctx);

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

	assert(ctx->blk_len > 0);

	OUT("Decoded header list:\n");
	res = 0;

	/* some compilers fail to see their proper initialization */
	len = 0;
	cb = NULL;

	do {
		if (res != 0)
			return (res);
		cut = 0;

		switch (*ctx->spec) {
		case '\0':
			cb = ctx->dec;
			len = ctx->blk_len;
			break;
		case 'a':
			abort();
			return (-1);
		case 'p':
			cut = 1;
			/* fall through */
		case 'd':
			ctx->spec++;

			cb = ctx->dec;
			len = atoi(ctx->spec);
			assert(len <= ctx->blk_len);
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

		res = cb(ctx->priv, ctx->blk, len, cut);

		if (cb == ctx->dec) {
			ctx->blk = (const uint8_t *)ctx->blk + len;
			ctx->blk_len -= len;
		}
	} while (ctx->blk_len > 0);

	return (res);
}

enum hpack_result_e
TST_translate_error(const char *str)
{
#define HPR(val, cod, txt, rst)			\
		if (!strcmp(str, #val))	\
			return (HPACK_RES_##val);
#include "tbl/hpack_tbl.h"
#undef HPR
	WRONG("Unknown error");
	return (INT_MAX);
}

/**********************************************************************
 * Signal handling
 */

static void
tst_dump_cb(void *priv, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	(void)vfprintf(priv, fmt, ap);
	va_end(ap);
}

static void
tst_sighandler(int signo)
{

	(void)signo;
	hpack_dump(hp, tst_dump_cb, stderr);

#ifdef NDEBUG
	exit(1);
#endif
}

void
TST_signal(void)
{

	(void)signal(SIGABRT, tst_sighandler);
}
