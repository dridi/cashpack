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
 * HPACK Indexing Tables (RFC 7541 Section 2.3)
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hpack.h"
#include "hpack_assert.h"
#include "hpack_priv.h"

#define MOVE(he, off)	(void *)((uintptr_t)(he) + (off))
#define JUMP(he, off)	(void *)((uintptr_t)(he) + sizeof *(he) + (off))
#define DIFF(a, b)	((uintptr_t)b - (uintptr_t)a)

const struct hpt_field hpt_static[] = {
#define HPS(n, v)				\
	{					\
		.nam = n,			\
		.val = (uint8_t*)v,		\
		.nam_sz = sizeof(n) - 1,	\
		.val_sz = sizeof(v) - 1,	\
	},
#include "tbl/hpack_static.h"
#undef HPS
};

/**********************************************************************
 * Tables lookups
 */

static struct hpt_entry *
hpt_dynamic(struct hpack *hp, size_t idx)
{
	struct hpt_entry *he;
	ptrdiff_t off;

	he = MOVE(hp->tbl, hp->off);
	off = he->pre_sz;

	assert(idx > 0);
	assert(idx <= hp->cnt);

	while (1) {
		assert(he->magic == HPT_ENTRY_MAGIC);
		assert(he->pre_sz == off);
		assert(he->nam_sz > 0);
		if (--idx == 0)
			return (he);
		off = sizeof *he + he->nam_sz + he->val_sz;
		he = JUMP(he, he->nam_sz + he->val_sz);
	}
}

int
HPT_search(HPACK_CTX, size_t idx, struct hpt_field *hf)
{
	const struct hpt_entry *he;

	if (idx <= HPT_STATIC_MAX) {
		memcpy(hf, &hpt_static[idx - 1], sizeof *hf);
		return (0);
	}

	idx -= HPT_STATIC_MAX;
	EXPECT(ctx, IDX, idx <= ctx->hp->cnt);

	he = hpt_dynamic(ctx->hp, idx);
	hf->nam_sz = he->nam_sz;
	hf->val_sz = he->val_sz;
	hf->nam = JUMP(he, 0);
	hf->val = JUMP(he, he->nam_sz);
	return (0);
}

void
HPT_foreach(HPACK_CTX)
{
	const struct hpt_entry *he, *tbl;
	ptrdiff_t off;
	size_t i;

	off = 0;
	tbl = ctx->hp->tbl;
	he = tbl;
	for (i = 0; i < ctx->hp->cnt; i++) {
		assert(DIFF(tbl, he) < ctx->hp->len);
		assert(he->magic == HPT_ENTRY_MAGIC);
		assert(he->pre_sz == off);
		assert(he->nam_sz > 0);
		off = sizeof *he + he->nam_sz + he->val_sz;
		CALLBACK(ctx, HPACK_EVT_FIELD, NULL, off);
		CALLBACK(ctx, HPACK_EVT_NAME, JUMP(he, 0), he->nam_sz);
		CALLBACK(ctx, HPACK_EVT_VALUE, JUMP(he, he->nam_sz),
		    he->val_sz);
		he = JUMP(he, he->nam_sz + he->val_sz);
	}
}

/**********************************************************************
 * Resize
 */

void
HPT_adjust(struct hpack *hp, size_t len)
{
	struct hpt_entry *he;
	size_t sz;

	if (hp->cnt == 0)
		return;

	he = hpt_dynamic(hp, hp->cnt);

	while (hp->cnt > 0 && len > hp->lim) {
		assert(he->magic == HPT_ENTRY_MAGIC);
		assert(he->nam_sz > 0);
		sz = sizeof *he + he->nam_sz + he->val_sz;
		len -= sz;
		hp->len -= sz;
		hp->cnt--;
		he = MOVE(he, -he->pre_sz);
	}

	if (hp->cnt == 0)
		assert(hp->len == 0);
	else
		assert(hp->len > 0);
}

/**********************************************************************
 * Insert
 */

static unsigned
hpt_notify(HPACK_CTX, enum hpack_evt_e evt, const void *buf, size_t len)
{

	switch (evt) {
	case HPACK_EVT_FIELD:
		assert(buf == NULL);
		assert(len == 32); /* entry overhead, see Section 4.1 */
		CALLBACK(ctx, evt, NULL, 0);
		return (1);
	case HPACK_EVT_DATA:
		assert(buf != NULL);
		/* fall through */
	case HPACK_EVT_NAME:
	case HPACK_EVT_VALUE:
		CALLBACK(ctx, evt, buf, len);
		break;
	default:
		WRONG("Unexpected event");
	}

	return (buf != NULL);
}

static unsigned
hpt_evict(struct hpt_priv *priv, size_t len)
{
	struct hpack *hp;

	hp = priv->ctx->hp;
	if (priv->ins) {
		priv->len += len;
		HPT_adjust(hp, hp->len + priv->len);
	}

	/* does the new field even fit alone? */
	return (priv->len > hp->lim);
}

static void
hpt_move(struct hpt_priv *priv, size_t len)
{
	struct hpack *hp;

	hp = priv->ctx->hp;
	priv->wrt = MOVE(hp->tbl, priv->len - len);

	if (hp->cnt == 0 || len == 0 || !priv->ins)
		return;

	hp->off += len;
	priv->he->pre_sz += len;
	priv->he = MOVE(priv->he, len);
	memmove(priv->he, priv->wrt, hp->len);
}

static void
hpt_copy(struct hpt_priv *priv, enum hpack_evt_e evt, const void *buf,
    size_t len)
{
	struct hpack *hp;

	hp = priv->ctx->hp;

	switch (evt) {
	case HPACK_EVT_FIELD:
		memset(hp->tbl, 0, sizeof *hp->tbl);
		hp->tbl->magic = HPT_ENTRY_MAGIC;
		break;
	case HPACK_EVT_VALUE:
		priv->nam = 0;
		/* fall through */
	case HPACK_EVT_NAME:
	case HPACK_EVT_DATA:
		break;
	default:
		WRONG("Unexpected event");
	}

	if (buf == NULL)
		return;

	if (priv->nam)
		hp->tbl->nam_sz += len;
	else
		hp->tbl->val_sz += len;
	memcpy(priv->wrt, buf, len);
}

void
HPT_insert(void *priv, enum hpack_evt_e evt, const void *buf, size_t len)
{
	struct hpt_priv *priv2;
	struct hpack *hp;

	assert(evt != HPACK_EVT_NEVER);
	assert(evt != HPACK_EVT_INDEX);
	assert(evt != HPACK_EVT_TABLE);

	priv2 = priv;
	hp = priv2->ctx->hp;

	priv2->ins = hpt_notify(priv2->ctx, evt, buf, len);

	if (hpt_evict(priv2, len)) {
		assert(hp->len == 0);
		assert(hp->cnt == 0);
		return;
	}

	hpt_move(priv2, len);
	hpt_copy(priv2, evt, buf, len);
}

/**********************************************************************
 * Decode
 */

int
HPT_decode(HPACK_CTX, size_t idx)
{
	struct hpt_field hf;

	assert(idx != 0);
	memset(&hf, 0, sizeof hf);
	CALL(HPT_search, ctx, idx, &hf);
	assert(hf.nam != NULL);
	assert(hf.val != NULL);
	CALLBACK(ctx, HPACK_EVT_NAME, hf.nam, hf.nam_sz);
	CALLBACK(ctx, HPACK_EVT_VALUE, hf.val, hf.val_sz);

	return (0);
}

int
HPT_decode_name(HPACK_CTX, size_t idx)
{
	struct hpt_field hf;

	assert(idx != 0);
	memset(&hf, 0, sizeof hf);
	CALL(HPT_search, ctx, idx, &hf);
	assert(hf.nam != NULL);
	CALLBACK(ctx, HPACK_EVT_NAME, hf.nam, hf.nam_sz);

	return (0);
}
