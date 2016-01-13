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
#include <stdint.h>
#include <stdlib.h>

#include "hpack.h"
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

	hp = calloc(1, sizeof *hp);
	if (hp == NULL)
		return (NULL);

	hp->magic = magic;
	hp->max = max;
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
	assert(hp->magic == ENCODER_MAGIC || hp->magic == DECODER_MAGIC);
	free(hp);
}

/**********************************************************************
 * Decoder
 */

static int
hpack_decode_indexed(HPACK_CTX)
{
	uint16_t idx;

	CALL(HPI_decode, ctx, HPACK_PFX_INDEXED, &idx);
	ctx->cb(ctx->priv, HPACK_EVT_FIELD, NULL, idx);
	return (HPT_decode(ctx, idx));
}

static int
hpack_decode_dynamic(HPACK_CTX)
{

	INCOMPL(ctx);
}

static int
hpack_decode_literal(HPACK_CTX)
{

	INCOMPL(ctx);
}

static int
hpack_decode_never(HPACK_CTX)
{

	INCOMPL(ctx);
}

static int
hpack_decode_update(HPACK_CTX)
{

	INCOMPL(ctx);
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
			return (ctx.res);
		}
	}

	assert(ctx.res == HPACK_RES_OK);
	return (ctx.res);
}
