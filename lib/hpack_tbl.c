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

#define HPT_OVERHEAD 32 /* section 4.1 */
#define HPT_HEADERSZ 30 /* account for 2 null bytes */

#define MOVE(he, off)	(void *)(uintptr_t)((uintptr_t)(he) + (off))
#define JUMP(he, off)	(void *)((uintptr_t)(he) + HPT_HEADERSZ + (off))
#define DIFF(a, b)	((uintptr_t)b - (uintptr_t)a)

const struct hpt_field hpt_static[] = {
#define HPS(n, v)				\
	{					\
		.nam = n,			\
		.val = v,			\
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
		off = HPT_OVERHEAD + he->nam_sz + he->val_sz;
		he = MOVE(he, off);
	}
}

int
HPT_search(HPACK_CTX, size_t idx, struct hpt_field *hf)
{
	const struct hpt_entry *he;

	if (idx <= HPT_STATIC_MAX) {
		(void)memcpy(hf, &hpt_static[idx - 1], sizeof *hf);
		return (0);
	}

	idx -= HPT_STATIC_MAX;
	EXPECT(ctx, IDX, idx <= ctx->hp->cnt);

	he = hpt_dynamic(ctx->hp, idx);
	hf->nam_sz = he->nam_sz;
	hf->val_sz = he->val_sz;
	hf->nam = JUMP(he, 0);
	hf->val = JUMP(he, he->nam_sz + 1);
	return (0);
}

void
HPT_foreach(HPACK_CTX)
{
	const struct hpt_entry *he, *tbl;
	ptrdiff_t off;
	size_t i;

	assert(ctx->hp->off == 0);

	off = 0;
	tbl = ctx->hp->tbl;
	he = tbl;
	for (i = 0; i < ctx->hp->cnt; i++) {
		assert(DIFF(tbl, he) < ctx->hp->len);
		assert(he->magic == HPT_ENTRY_MAGIC);
		assert(he->pre_sz == off);
		assert(he->nam_sz > 0);
		off = HPT_OVERHEAD + he->nam_sz + he->val_sz;
		CALLBACK(ctx, HPACK_EVT_FIELD, NULL, off);
		CALLBACK(ctx, HPACK_EVT_NAME, JUMP(he, 0), he->nam_sz);
		CALLBACK(ctx, HPACK_EVT_VALUE, JUMP(he, he->nam_sz + 1),
		    he->val_sz);
		he = MOVE(he, off);
	}
}

/**********************************************************************
 * Resize
 */

void
HPT_adjust(struct hpack_ctx *ctx, size_t len)
{
	struct hpack *hp;
	struct hpt_entry *he;
	size_t sz;

	hp = ctx->hp;
	if (hp->cnt == 0)
		return;

	he = hpt_dynamic(hp, hp->cnt);

	while (hp->cnt > 0 && len > hp->lim) {
		assert(he->magic == HPT_ENTRY_MAGIC);
		assert(he->nam_sz > 0);
		sz = HPT_OVERHEAD + he->nam_sz + he->val_sz;
		len -= sz;
		hp->len -= sz;
		hp->cnt--;
		he = MOVE(he, -he->pre_sz);
		CALLBACK(ctx, HPACK_EVT_EVICT, NULL, 0);
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
hpt_notify(struct hpt_priv *priv, enum hpack_evt_e evt, const char *buf,
    size_t len)
{
	struct hpack *hp;
	char *c;

	hp = priv->ctx->hp;

	switch (evt) {
	case HPACK_EVT_NAME:
		assert(len > 0);

		assert(priv->he == hp->tbl);
		assert(priv->he->pre_sz == 0);
		assert(hp->off == 0);

		assert(sizeof *hp->tbl == HPT_OVERHEAD);
		/* account for the entry header + the name null byte */
		priv->he->pre_sz = HPT_HEADERSZ + 1;
		priv->len = HPT_HEADERSZ + 1;
		priv->nam = 1;
		break;
	case HPACK_EVT_VALUE:
		/* write the name null byte */
		c = MOVE(hp->tbl, priv->len - 1);
		*c = '\0';
		/* account for the value null byte */
		priv->len++;
		priv->nam = 0;
		break;
	case HPACK_EVT_DATA:
		assert(buf != NULL);
		assert(len > 0);
		break;
	case HPACK_EVT_INDEX:
		assert(buf == NULL);
		assert(len == 0);
		/* write the value null byte */
		c = MOVE(hp->tbl, priv->len - 1);
		*c = '\0';
		break;
	default:
		WRONG("Unexpected event");
	}

	CALLBACK(priv->ctx, evt, buf, len);

	/* is there anything to copy in the table? */
	return (evt == HPACK_EVT_NAME || (buf != NULL && len > 0));
}

static unsigned
hpt_evict(struct hpt_priv *priv, const char *buf, size_t len)
{
	struct hpack *hp;

	hp = priv->ctx->hp;
	if (buf != NULL)
		priv->len += len;
	HPT_adjust(priv->ctx, hp->len + priv->len);

	/* does the new field even fit alone? */
	if (priv->len > hp->lim) {
		assert(hp->len == 0);
		assert(hp->cnt == 0);
		return (0);
	}
	return (1);
}

static unsigned
hpt_overlap(const struct hpack *hp, const char *buf, size_t len)
{
	uintptr_t bgn, end, pos;

	if (buf == NULL)
		return (0);

	bgn = (uintptr_t)hp->tbl;
	pos = (uintptr_t)buf;
	end = bgn + hp->len;

	if (pos >= bgn && pos < end) {
		pos += len;
		assert(pos >= bgn && pos < end);
		return (1);
	}

	pos += len;
	assert(pos < bgn || pos >= end);
	return (0);
}

static void
hpt_copy(struct hpt_priv *priv, const char *buf, size_t len)
{
	struct hpack *hp;
	void *tmp;

	hp = priv->ctx->hp;

	if (hp->cnt > 0) {
		assert(priv->he->magic == HPT_ENTRY_MAGIC);
		tmp = priv->he;
		hp->off = priv->len;
		priv->he->pre_sz = priv->len;
		priv->he = MOVE(hp->tbl, priv->he->pre_sz);
		(void)memmove(priv->he, tmp, hp->len);
		assert(priv->he->magic == HPT_ENTRY_MAGIC);
	}

	if (hpt_overlap(hp, buf, len))
		buf += priv->len;

	if (buf != NULL)
		(void)memcpy(priv->wrt, buf, len);
}

void
HPT_insert(void *priv, enum hpack_evt_e evt, const char *buf, size_t len)
{
	struct hpt_priv *priv2;
	struct hpack *hp;
	unsigned ovl;

	assert(evt != HPACK_EVT_NEVER);
	assert(evt != HPACK_EVT_EVICT);
	assert(evt != HPACK_EVT_TABLE);

	priv2 = priv;
	hp = priv2->ctx->hp;

	ovl = hpt_overlap(hp, buf, len);
	if (ovl) {
		assert(evt == HPACK_EVT_NAME);
		assert(hp->cnt > 0);
	}

	if (!hpt_notify(priv2, evt, buf, len) || !hpt_evict(priv2, buf, len))
		return;

	priv2->wrt = evt == HPACK_EVT_NAME ?
	    JUMP(hp->tbl, 0) :
	    MOVE(hp->tbl, priv2->len - len - 1);

	if (ovl && !hpt_overlap(hp, buf, len))
		INCOMPL(); /* XXX: insert evicted indexed name */
	else
		hpt_copy(priv2, buf, len);

	if (evt == HPACK_EVT_NAME) {
		if (hp->cnt > 0) {
			assert(priv2->he != hp->tbl);
			assert(priv2->he->pre_sz >= HPT_HEADERSZ);
		}
		(void)memset(hp->tbl, 0, HPT_HEADERSZ);
		hp->tbl->magic = HPT_ENTRY_MAGIC;
	}

	if (buf == NULL)
		return;

	if (priv2->nam)
		hp->tbl->nam_sz += len;
	else
		hp->tbl->val_sz += len;
}

/**********************************************************************
 * Decode
 */

int
HPT_decode(HPACK_CTX, size_t idx)
{
	struct hpt_field hf;

	assert(idx != 0);
	(void)memset(&hf, 0, sizeof hf);
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
	(void)memset(&hf, 0, sizeof hf);
	CALL(HPT_search, ctx, idx, &hf);
	assert(hf.nam != NULL);
	CALLBACK(ctx, HPACK_EVT_NAME, hf.nam, hf.nam_sz);

	return (0);
}
