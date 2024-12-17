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
 *
 * HPACK decoding.
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hpack.h"
#include "hpack_priv.h"

static int
hpd_skip(HPACK_CTX, size_t len)
{
	size_t fld_len;

	if (ctx->buf_len >= len)
		return (0);

	ctx->flg |= HPACK_CTX_TOO_BIG;
	EXPECT(ctx, BIG, ctx->fld.nam != ctx->arg.dec->buf);

	assert(ctx->fld.nam != NULL);
	assert(ctx->fld.nam <= ctx->buf);
	if (ctx->fld.val != NULL)
		assert(ctx->fld.val <= ctx->buf);
	fld_len = (size_t)(ctx->buf - ctx->fld.nam);

	EXPECT(ctx, BIG, ctx->arg.dec->buf_len >= len + fld_len);

	memmove(ctx->arg.dec->buf, ctx->fld.nam, fld_len);

	ctx->buf = ctx->arg.dec->buf;
	ctx->buf += fld_len;
	ctx->buf_len = ctx->arg.dec->buf_len - fld_len;

	ctx->fld.nam = ctx->arg.dec->buf;
	if (ctx->fld.val != NULL)
		ctx->fld.val = ctx->fld.nam + ctx->fld.nam_sz + 1;
	assert((ssize_t)fld_len == ctx->buf - ctx->fld.nam);

	return (0);
}

int
HPD_putc(HPACK_CTX, char c)
{

	CALL(hpd_skip, ctx, 1);
	*ctx->buf = c;
	ctx->buf++;
	ctx->buf_len--;
	return (0);
}

int
HPD_puts(HPACK_CTX, const char *str, size_t len)
{

#define HPD_UNKNOWN(prop, sym)					\
	if (str == hpack_unknown_##sym) {			\
		assert(ctx->hp->magic == DECODER_MAGIC);	\
		assert(ctx->hp->flg & HPD_FLG_MON);		\
		assert(ctx->fld.prop == ctx->buf);		\
		assert(ctx->fld.prop##_sz == len);		\
		ctx->fld.prop = str;				\
		return (0);					\
	}
	HPD_UNKNOWN(nam, name)
	HPD_UNKNOWN(val, value)
#undef HPD_UNKNOWN

	assert(str[len] == '\0');
	return (HPD_cat(ctx, str, len + 1));
}

int
HPD_cat(HPACK_CTX, const char *str, size_t len)
{

	CALL(hpd_skip, ctx, len);
	(void)memcpy(ctx->buf, str, len);
	ctx->buf += len;
	ctx->buf_len -= len;
	return (0);
}

void
HPD_notify(HPACK_CTX)
{

	assert(ctx->fld.nam != NULL);
	assert(ctx->fld.val != NULL);
	assert(ctx->fld.nam_sz > 0);
	assert(ctx->fld.nam[ctx->fld.nam_sz] == '\0');
	assert(ctx->fld.val[ctx->fld.val_sz] == '\0');

	HPC_notify(ctx, HPACK_EVT_NAME,  ctx->fld.nam, ctx->fld.nam_sz);
	HPC_notify(ctx, HPACK_EVT_VALUE, ctx->fld.val, ctx->fld.val_sz);
}
