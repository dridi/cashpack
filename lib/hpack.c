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
 *
 * HPACK: Header Compression for HTTP/2 (RFC 7541)
 */

#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hpack.h"
#include "hpack_assert.h"
#include "hpack_priv.h"

enum hpack_pfx_e {
	HPACK_PFX_STRING	= 7,
	HPACK_PFX_INDEXED	= 7,
	HPACK_PFX_DYNAMIC	= 6,
	HPACK_PFX_LITERAL	= 4,
	HPACK_PFX_NEVER		= 4,
	HPACK_PFX_UPDATE	= 5,
};

/**********************************************************************
 */

static struct hpack *
hpack_new(uint32_t magic, size_t max)
{
	struct hpack *hp;

	hp = calloc(1, sizeof *hp + max);
	if (hp == NULL)
		return (NULL);

	hp->magic = magic;
	hp->max = max;
	hp->lim = max;
	return (hp);
}

struct hpack *
HPACK_encoder(size_t max)
{

	return (hpack_new(ENCODER_MAGIC, max));
}

struct hpack *
HPACK_decoder(size_t max)
{

	return (hpack_new(DECODER_MAGIC, max));
}

void
HPACK_free(struct hpack **hpp)
{
	struct hpack *hp;

	assert(hpp != NULL);
	hp = *hpp;
	if (hp == NULL)
		return;

	*hpp = NULL;
	assert(hp->magic == ENCODER_MAGIC || hp->magic == DECODER_MAGIC ||
	    hp->magic == DEFUNCT_MAGIC);
	free(hp);
}

int
HPACK_foreach(struct hpack *hp, hpack_decoded_f cb, void *priv)
{
	struct hpack_ctx ctx;

	if (hp == NULL || cb == NULL)
		return (-1);
	if (hp->magic != DECODER_MAGIC && hp->magic != ENCODER_MAGIC)
		return (-1);

	memset(&ctx, 0, sizeof ctx);
	ctx.hp = hp;
	ctx.cb = cb;
	ctx.priv = priv;

	HPT_foreach(&ctx);
	return (0);
}

/**********************************************************************
 * Decoder
 */

static int
hpack_decode_string(HPACK_CTX, enum hpack_evt_e evt)
{
	uint16_t len;
	uint8_t huf;
	hpack_validate_f *val;

	huf = *ctx->buf & HPACK_HUFFMAN;
	CALL(HPI_decode, ctx, HPACK_PFX_STRING, &len);
	EXPECT(ctx, BUF, ctx->len >= len);

	if (evt == HPACK_EVT_NAME) {
		EXPECT(ctx, LEN, len > 0);
		val = HPV_token;
	}
	else
		val = HPV_value;

	if (huf) {
		CALLBACK(ctx, evt, NULL, len);
		CALL(HPH_decode, ctx, val, len);
	}
	else {
		CALL(val, ctx, (char *)ctx->buf, len, 1);
		CALLBACK(ctx, evt, (char *)ctx->buf, len);
		ctx->buf += len;
		ctx->len -= len;
	}

	return (0);
}

static int
hpack_decode_field(HPACK_CTX, uint16_t idx)
{

	if (idx == 0)
		CALL(hpack_decode_string, ctx, HPACK_EVT_NAME);
	else
		CALL(HPT_decode_name, ctx, idx);

	return (hpack_decode_string(ctx, HPACK_EVT_VALUE));
}

static int
hpack_decode_indexed(HPACK_CTX)
{
	uint16_t idx;

	CALL(HPI_decode, ctx, HPACK_PFX_INDEXED, &idx);
	CALLBACK(ctx, HPACK_EVT_FIELD, NULL, idx);
	return (HPT_decode(ctx, idx));
}

static int
hpack_decode_dynamic(HPACK_CTX)
{
	struct hpack *hp;
	struct hpack_ctx tbl_ctx;
	struct hpt_priv priv;
	uint16_t idx;

	hp = ctx->hp;

	memset(&priv, 0, sizeof priv);
	priv.ctx = ctx;
	priv.he = hp->tbl;

	CALL(HPI_decode, ctx, HPACK_PFX_DYNAMIC, &idx);
	CALLBACK(ctx, HPACK_EVT_FIELD, NULL, 0);

	memcpy(&tbl_ctx, ctx, sizeof tbl_ctx);
	tbl_ctx.cb = HPT_insert;
	tbl_ctx.priv = &priv;

	if (hpack_decode_field(&tbl_ctx, idx) != 0) {
		assert(tbl_ctx.res != HPACK_RES_OK);
		ctx->res = tbl_ctx.res;
		return (-1);
	}

	assert(tbl_ctx.res == HPACK_RES_OK);
	ctx->buf = tbl_ctx.buf;
	ctx->len = tbl_ctx.len;
	hp->off = 0;

	if (priv.len <= hp->lim) {
		hp->len += priv.len;
		if (++ctx->hp->cnt > 1)
			assert(priv.he->pre_sz = priv.len);
	}
	else {
		assert(hp->len == 0);
		assert(hp->cnt == 0);
	}

	return (0);
}

static int
hpack_decode_literal(HPACK_CTX)
{
	uint16_t idx;

	CALL(HPI_decode, ctx, HPACK_PFX_LITERAL, &idx);
	CALLBACK(ctx, HPACK_EVT_FIELD, NULL, 0);
	return (hpack_decode_field(ctx, idx));
}

static int
hpack_decode_never(HPACK_CTX)
{
	uint16_t idx;

	CALL(HPI_decode, ctx, HPACK_PFX_NEVER, &idx);
	EXPECT(ctx, IDX, idx == 0);
	CALLBACK(ctx, HPACK_EVT_FIELD, NULL, 0);
	CALLBACK(ctx, HPACK_EVT_NEVER, NULL, 0);
	return (hpack_decode_field(ctx, idx));
}

static int
hpack_decode_update(HPACK_CTX)
{
	uint16_t sz;

	CALL(HPI_decode, ctx, HPACK_PFX_UPDATE, &sz);
	EXPECT(ctx, LEN, sz <= ctx->hp->max);
	ctx->hp->lim = sz;
	HPT_adjust(ctx->hp, ctx->hp->len);
	return (0);
}

enum hpack_res_e
HPACK_decode(struct hpack *hp, const void *buf, size_t len,
    hpack_decoded_f cb, void *priv)
{
	struct hpack_ctx ctx;
	int retval;

	if (hp == NULL || hp->magic != DECODER_MAGIC || buf == NULL ||
	    len == 0 || cb == NULL)
		return (HPACK_RES_ARG);

	ctx.res = HPACK_RES_OK;
	ctx.hp = hp;
	ctx.buf = buf;
	ctx.len = len;
	ctx.cb = cb;
	ctx.priv = priv;

	while (ctx.len > 0) {
#define HPACK_DECODE(l, U, or) 					\
		if ((*ctx.buf & HPACK_##U) == HPACK_##U)	\
			retval = hpack_decode_##l(&ctx); 	\
		or
		HPACK_DECODE(indexed, INDEXED, else)
		HPACK_DECODE(dynamic, DYNAMIC, else)
		HPACK_DECODE(update,  UPDATE,  else)
		HPACK_DECODE(never,   NEVER,   else)
		HPACK_DECODE(literal, LITERAL, /* out of bits */)
#undef HPACK_DECODE
		if (retval != 0) {
			assert(ctx.res != HPACK_RES_OK);
			hp->magic = DEFUNCT_MAGIC;
			return (ctx.res);
		}
	}

	assert(ctx.res == HPACK_RES_OK);
	return (ctx.res);
}
