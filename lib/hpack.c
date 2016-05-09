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

const struct hpack_alloc hpack_libc_alloc = { malloc, realloc, free };

const struct hpack_alloc *hpack_default_alloc = &hpack_libc_alloc;

/**********************************************************************
 */

static struct hpack *
hpack_new(uint32_t magic, size_t max, const struct hpack_alloc *ha)
{
	struct hpack *hp;

	if (ha == NULL || ha->malloc == NULL || max > UINT16_MAX)
		return (NULL);

	hp = ha->malloc(sizeof *hp + max);
	if (hp == NULL)
		return (NULL);

	(void)memset(hp, 0, sizeof *hp + max);
	hp->magic = magic;
	(void)memcpy(&hp->alloc, ha, sizeof *ha);
	hp->mem = max;
	hp->max = max;
	hp->lim = max;
	hp->nxt = -1;
	hp->min = -1;
	return (hp);
}

struct hpack *
hpack_encoder(size_t max, const struct hpack_alloc *ha)
{

	return (hpack_new(ENCODER_MAGIC, max, ha));
}

struct hpack *
hpack_decoder(size_t max, const struct hpack_alloc *ha)
{

	return (hpack_new(DECODER_MAGIC, max, ha));
}

enum hpack_res_e
hpack_resize(struct hpack **hpp, size_t len)
{
	struct hpack *hp;
	size_t max;

	if (hpp == NULL)
		return (HPACK_RES_ARG);

	hp = *hpp;
	if (hp == NULL)
		return (HPACK_RES_ARG);
	if (hp->magic != DECODER_MAGIC && hp->magic != ENCODER_MAGIC)
		return (HPACK_RES_ARG);

	max = hp->alloc.realloc == NULL ? hp->mem : UINT16_MAX;
	if (len > max) {
		hp->magic = DEFUNCT_MAGIC;
		return (HPACK_RES_LEN);
	}

	if (len > hp->mem) {
		assert(hp->alloc.realloc != NULL);
		hp = hp->alloc.realloc(hp, sizeof *hp + len);
		if (hp == NULL) {
			(*hpp)->magic = DEFUNCT_MAGIC;
			return (HPACK_RES_OOM);
		}
		hp->mem = len;
		*hpp = hp;
	}

	if (hp->min < 0) {
		assert(hp->nxt < 0);
		hp->nxt = len;
		hp->min = len;
	}
	else {
		assert(hp->nxt >= hp->min);
		hp->nxt = len;
		if (hp->min > (ssize_t)len)
			hp->min = len;
	}

	return (HPACK_RES_OK);
}

enum hpack_res_e
hpack_trim(struct hpack **hpp)
{
	struct hpack *hp;

	if (hpp == NULL)
		return (HPACK_RES_ARG);

	hp = *hpp;
	if (hp == NULL || hp->alloc.realloc == NULL)
		return (HPACK_RES_ARG);
	if (hp->magic != DECODER_MAGIC && hp->magic != ENCODER_MAGIC)
		return (HPACK_RES_ARG);

	if (hp->mem > hp->max) {
		hp = hp->alloc.realloc(hp, hp->max);
		if (hp == NULL)
			return (HPACK_RES_OOM); /* the codec is NOT defunct */
		hp->mem = hp->max;
		*hpp = hp;
	}

	return (HPACK_RES_OK);
}

void
hpack_free(struct hpack **hpp)
{
	struct hpack *hp;

	if (hpp == NULL)
		return;

	hp = *hpp;
	if (hp == NULL)
		return;

	*hpp = NULL;
	assert(hp->magic == ENCODER_MAGIC || hp->magic == DECODER_MAGIC ||
	    hp->magic == DEFUNCT_MAGIC);

	hp->magic = 0;
	if (hp->alloc.free != NULL)
		hp->alloc.free(hp);
}

int
hpack_foreach(struct hpack *hp, hpack_decoded_f cb, void *priv)
{
	struct hpack_ctx ctx;

	if (hp == NULL || cb == NULL)
		return (HPACK_RES_ARG);
	if (hp->magic != DECODER_MAGIC && hp->magic != ENCODER_MAGIC)
		return (HPACK_RES_ARG);

	(void)memset(&ctx, 0, sizeof ctx);
	ctx.hp = hp;
	ctx.dec = cb;
	ctx.priv = priv;

	HPT_foreach(&ctx);
	return (HPACK_RES_OK);
}

const char *
hpack_strerror(enum hpack_res_e res)
{

#define HPR(val, cod, txt)	\
	if (res == cod)		\
		return (txt);
#include "tbl/hpack_tbl.h"
#undef HPR
	return ("Unknown result");
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
#ifndef NDEBUG
	struct hpt_entry tmp;
#endif

	hp = ctx->hp;

	(void)memset(&priv, 0, sizeof priv);
	priv.ctx = ctx;
	priv.he = hp->tbl;

	CALL(HPI_decode, ctx, HPACK_PFX_DYNAMIC, &idx);
	CALLBACK(ctx, HPACK_EVT_FIELD, NULL, 0);

	(void)memcpy(&tbl_ctx, ctx, sizeof tbl_ctx);
	tbl_ctx.dec = HPT_insert;
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
		CALLBACK(&tbl_ctx, HPACK_EVT_INDEX, NULL, 0);
		hp->len += priv.len;
		if (++ctx->hp->cnt > 1) {
#ifndef NDEBUG
			(void)memcpy(&tmp, priv.he, sizeof tmp);
			assert(tmp.pre_sz > 0);
			assert((size_t)tmp.pre_sz == priv.len);
#endif
		}
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
	CALLBACK(ctx, HPACK_EVT_FIELD, NULL, 0);
	CALLBACK(ctx, HPACK_EVT_NEVER, NULL, 0);
	return (hpack_decode_field(ctx, idx));
}

static int
hpack_decode_update(HPACK_CTX)
{
	uint16_t sz;

	EXPECT(ctx, UPD, ctx->can_upd);

	CALL(HPI_decode, ctx, HPACK_PFX_UPDATE, &sz);
	if (ctx->hp->min >= 0) {
		assert(ctx->hp->min <= ctx->hp->nxt);
		if (ctx->hp->min < ctx->hp->nxt) {
			EXPECT(ctx, UPD, sz == ctx->hp->min);
			ctx->hp->min = ctx->hp->nxt;
		}
		else {
			EXPECT(ctx, UPD, sz == ctx->hp->nxt ||
			    sz < ctx->hp->nxt);
			ctx->hp->max = ctx->hp->nxt;
			ctx->hp->lim = sz;
			ctx->hp->min = -1;
			ctx->hp->nxt = -1;
			ctx->can_upd = 0;
		}
	}
	else
		ctx->can_upd = 0;
	EXPECT(ctx, LEN, sz <= ctx->hp->max);
	ctx->hp->lim = sz;
	HPT_adjust(ctx, ctx->hp->len);
	CALLBACK(ctx, HPACK_EVT_TABLE, NULL, sz);
	return (0);
}

enum hpack_res_e
hpack_decode(struct hpack *hp, const void *buf, size_t len, unsigned cut,
    hpack_decoded_f cb, void *priv)
{
	struct hpack_ctx ctx;
	int retval;

	if (hp == NULL || hp->magic != DECODER_MAGIC || buf == NULL ||
	    len == 0 || cb == NULL)
		return (HPACK_RES_ARG);

	if (cut)
		INCOMPL();

	ctx.res = HPACK_RES_OK;
	ctx.hp = hp;
	ctx.buf = buf;
	ctx.len = len;
	ctx.dec = cb;
	ctx.priv = priv;
	ctx.can_upd = 1;

	while (ctx.len > 0) {
		if ((*ctx.buf & HPACK_UPDATE) != HPACK_UPDATE) {
			EXPECT(&ctx, RSZ, hp->nxt < 0);
			assert(hp->min < 0);
			ctx.can_upd = 0;
		}
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

/**********************************************************************
 * Encoder
 */

static int
hpack_encode_string(HPACK_CTX, HPACK_ITM, enum hpack_evt_e evt)
{
	const char *str;
	size_t len;
	unsigned huf;
	hpack_validate_f *val;

	if (evt == HPACK_EVT_NAME) {
		assert(~itm->fld.flg & HPACK_IDX);
		str = itm->fld.nam;
		huf = itm->fld.flg & HPACK_NAM;
		val = HPV_token;
	}
	else {
		str = itm->fld.val;
		huf = itm->fld.flg & HPACK_VAL;
		val = HPV_value;
	}

	len = strlen(str);
	CALL(val, ctx, str, len, 1);

	if (huf != 0) {
		HPH_size(str, &len);
		HPI_encode(ctx, HPACK_PFX_STRING, HPACK_HUFFMAN, len);
		HPH_encode(ctx, str);
	}
	else {
		HPI_encode(ctx, HPACK_PFX_STRING, HPACK_RAW, len);
		HPE_push(ctx, str, len);
	}

	return (0);
}

static int
hpack_encode_field(HPACK_CTX, HPACK_ITM, size_t pfx)
{
	uint16_t idx;

	if (itm->fld.flg & HPACK_IDX) {
		idx = itm->fld.idx;
		EXPECT(ctx, IDX, idx > 0 &&
		    idx <= ctx->hp->cnt + HPT_STATIC_MAX);
	}
	else
		idx = 0;

	HPI_encode(ctx, pfx, itm->typ, idx);

	if (idx == 0)
		CALL(hpack_encode_string, ctx, itm, HPACK_EVT_NAME);

	return (hpack_encode_string(ctx, itm, HPACK_EVT_VALUE));
}

static int
hpack_encode_indexed(HPACK_CTX, HPACK_ITM)
{

	EXPECT(ctx, IDX, itm->idx > 0 &&
	    itm->idx <= ctx->hp->cnt + HPT_STATIC_MAX);

	HPI_encode(ctx, HPACK_PFX_INDEXED, itm->typ, itm->idx);
	return (0);
}

static int
hpack_encode_dynamic(HPACK_CTX, HPACK_ITM)
{
	struct hpack *hp;
	struct hpt_field hf;
	struct hpt_priv priv;
	size_t nam_sz, val_sz;
#ifndef NDEBUG
	struct hpt_entry tmp;
#endif

	CALL(hpack_encode_field, ctx, itm, HPACK_PFX_DYNAMIC);

	hp = ctx->hp;

	(void)memset(&priv, 0, sizeof priv);
	priv.ctx = ctx;
	priv.he = hp->tbl;
	priv.enc = 1;

	if (itm->fld.flg & HPACK_IDX) {
		(void)HPT_search(ctx, itm->fld.idx, &hf);
		assert(ctx->res == HPACK_RES_OK);
		HPT_insert(&priv, HPACK_EVT_NAME, hf.nam, hf.nam_sz);
	}
	else {
		nam_sz = strlen(itm->fld.nam);
		HPT_insert(&priv, HPACK_EVT_NAME, itm->fld.nam, nam_sz);
	}

	val_sz = strlen(itm->fld.val);
	HPT_insert(&priv, HPACK_EVT_VALUE, itm->fld.val, val_sz);

	hp->off = 0;

	if (priv.len <= hp->lim) {
		HPT_insert(&priv, HPACK_EVT_INDEX, NULL, 0);
		hp->len += priv.len;
		if (++ctx->hp->cnt > 1) {
#ifndef NDEBUG
			(void)memcpy(&tmp, priv.he, sizeof tmp);
			assert(tmp.pre_sz > 0);
			assert((size_t)tmp.pre_sz == priv.len);
#endif
		}
	}

	return (0);
}

static int
hpack_encode_literal(HPACK_CTX, HPACK_ITM)
{

	return (hpack_encode_field(ctx, itm, HPACK_PFX_LITERAL));
}

static int
hpack_encode_never(HPACK_CTX, HPACK_ITM)
{

	return (hpack_encode_field(ctx, itm, HPACK_PFX_NEVER));
}

static int
hpack_encode_update(HPACK_CTX, HPACK_ITM)
{

	EXPECT(ctx, UPD, ctx->can_upd);
	EXPECT(ctx, ARG, itm->lim <= UINT16_MAX);
	EXPECT(ctx, LEN, itm->lim <= ctx->hp->max);
	if (ctx->hp->min >= 0) {
		assert(ctx->hp->min <= ctx->hp->nxt);
		if (ctx->hp->min < ctx->hp->nxt)
			assert(itm->lim == (size_t)ctx->hp->min);
	}
	ctx->hp->lim = itm->lim;
	HPT_adjust(ctx, ctx->hp->len);
	HPI_encode(ctx, HPACK_PFX_UPDATE, itm->typ, itm->lim);
	CALLBACK(ctx, HPACK_EVT_TABLE, NULL, itm->lim);

	if (ctx->hp->min < ctx->hp->nxt) {
		assert(ctx->hp->min >= 0);
		ctx->hp->min = ctx->hp->nxt;
	}
	else if (ctx->hp->min >= 0) {
		assert(ctx->hp->min == ctx->hp->nxt);
		ctx->hp->max = ctx->hp->nxt;
		ctx->hp->lim = ctx->hp->nxt;
		ctx->hp->min = -1;
		ctx->hp->nxt = -1;
		ctx->can_upd = 0;
	}

	return (0);
}

enum hpack_res_e
hpack_encode(struct hpack *hp, HPACK_ITM, size_t len, hpack_encoded_f cb,
    void *priv)
{
	struct hpack_ctx ctx;
	struct hpack_item rsz_itm;
	uint8_t buf[256];
	int retval;

	if (hp == NULL || hp->magic != ENCODER_MAGIC || itm == NULL ||
	    len == 0 || cb == NULL)
		return (HPACK_RES_ARG);

	ctx.res = HPACK_RES_OK;
	ctx.hp = hp;
	ctx.buf = buf;
	ctx.cur = TRUST_ME(ctx.buf);
	ctx.len = 0;
	ctx.max = sizeof buf;
	ctx.enc	= cb;
	ctx.priv = priv;
	ctx.can_upd = 1;

	if (hp->min >= 0) {
		assert(hp->min <= hp->nxt);
		(void)memset(&rsz_itm, 0, sizeof rsz_itm);
		rsz_itm.typ = HPACK_UPDATE;
		rsz_itm.lim = hp->min;
		retval = hpack_encode_update(&ctx, &rsz_itm);
		assert(retval == 0);
		assert(hp->min == hp->nxt);
		if (hp->nxt >= 0) {
			rsz_itm.lim = hp->nxt;
			retval = hpack_encode_update(&ctx, &rsz_itm);
			assert(retval == 0);
		}
		hpack_clean_item(&rsz_itm);
	}

	while (len > 0) {
		if (itm->typ != HPACK_UPDATE)
			ctx.can_upd = 0;
#define HPACK_ENCODE(l, U, or) 					\
		if (itm->typ == HPACK_##U)			\
			retval = hpack_encode_##l(&ctx, itm); 	\
		or
		HPACK_ENCODE(indexed, INDEXED, else)
		HPACK_ENCODE(dynamic, DYNAMIC, else)
		HPACK_ENCODE(literal, LITERAL, else)
		HPACK_ENCODE(never,   NEVER,   else)
		HPACK_ENCODE(update,  UPDATE,  /* that was the last */)
		else {
			hp->magic = DEFUNCT_MAGIC;
			return (HPACK_RES_ARG);
		}
#undef HPACK_ENCODE
		if (retval != 0) {
			assert(ctx.res != HPACK_RES_OK);
			hp->magic = DEFUNCT_MAGIC;
			return (ctx.res);
		}
		itm++;
		len--;
	}

	HPE_send(&ctx);

	assert(ctx.res == HPACK_RES_OK);
	return (ctx.res);
}

enum hpack_res_e
hpack_clean_item(struct hpack_item *itm)
{

	if (itm == NULL)
		return (HPACK_RES_ARG);

	switch (itm->typ) {
	case HPACK_UPDATE:
		itm->lim = 0;
		break;
	case HPACK_INDEXED:
		itm->idx = 0;
		break;
	case HPACK_DYNAMIC:
	case HPACK_LITERAL:
	case HPACK_NEVER:
		if (itm->fld.flg & HPACK_IDX) {
			itm->fld.idx = 0;
			itm->fld.flg &= ~HPACK_IDX;
		}
		else
			itm->fld.nam = NULL;
		itm->fld.val = NULL;
		itm->fld.flg &= ~HPACK_NAM;
		itm->fld.flg &= ~HPACK_VAL;
		break;
		break;
	default:
		return (HPACK_RES_ARG);
	}

	itm->typ = 0;

	if (itm->fld.nam != NULL || itm->fld.val != NULL || itm->fld.flg != 0)
		return (HPACK_RES_ARG);

	assert(itm->idx == 0);
	assert(itm->lim == 0);
	assert(itm->fld.idx == 0);

	return (HPACK_RES_OK);
}
