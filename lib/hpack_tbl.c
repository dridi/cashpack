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
	struct hpt_entry *he, tmp;
	ptrdiff_t off;

	he = MOVE(hp->tbl, hp->off);
	(void)memcpy(&tmp, he, HPT_HEADERSZ);
	off = tmp.pre_sz;

	assert(idx > 0);
	assert(idx <= hp->cnt);

	while (1) {
		assert(tmp.magic == HPT_ENTRY_MAGIC);
		assert(tmp.pre_sz == off);
		assert(tmp.nam_sz > 0);
		if (--idx == 0)
			return (he);
		off = HPT_OVERHEAD + tmp.nam_sz + tmp.val_sz;
		he = MOVE(he, off);
		(void)memcpy(&tmp, he, HPT_HEADERSZ);
	}
}

int
HPT_search(HPACK_CTX, size_t idx, struct hpt_field *hf)
{
	const struct hpt_entry *he;
	struct hpt_entry tmp;

	assert(idx != 0);
	if (idx <= HPACK_STATIC) {
		(void)memcpy(hf, &hpt_static[idx - 1], sizeof *hf);
		return (0);
	}

	idx -= HPACK_STATIC;
	EXPECT(ctx, IDX, idx <= ctx->hp->cnt);

	he = hpt_dynamic(ctx->hp, idx);
	(void)memcpy(&tmp, he, HPT_HEADERSZ);
	hf->nam_sz = tmp.nam_sz;
	hf->val_sz = tmp.val_sz;
	hf->nam = JUMP(he, 0);
	hf->val = JUMP(he, tmp.nam_sz + 1);
	return (0);
}

void
HPT_foreach(HPACK_CTX, int flg)
{
	const struct hpt_entry *he, *tbl;
	const struct hpt_field *hf;
	struct hpt_entry tmp;
	ptrdiff_t off;
	size_t i;

	if (flg & HPT_FLG_STATIC)
		for (i = 0, hf = hpt_static; i < HPACK_STATIC; i++, hf++) {
			CALLBACK(ctx, HPACK_EVT_FIELD, NULL, 0);
			CALLBACK(ctx, HPACK_EVT_NAME, hf->nam, hf->nam_sz);
			CALLBACK(ctx, HPACK_EVT_VALUE, hf->val, hf->val_sz);
		}

	if (~flg & HPT_FLG_DYNAMIC)
		return;

	assert(ctx->hp->off == 0);

	off = 0;
	tbl = ctx->hp->tbl;
	he = tbl;
	for (i = 0; i < ctx->hp->cnt; i++) {
		assert(DIFF(tbl, he) < ctx->hp->sz.len);
		(void)memcpy(&tmp, he, HPT_HEADERSZ);
		assert(tmp.magic == HPT_ENTRY_MAGIC);
		assert(tmp.pre_sz == off);
		assert(tmp.nam_sz > 0);
		off = HPT_OVERHEAD + tmp.nam_sz + tmp.val_sz;
		CALLBACK(ctx, HPACK_EVT_FIELD, NULL, off);
		CALLBACK(ctx, HPACK_EVT_NAME, JUMP(he, 0), tmp.nam_sz);
		CALLBACK(ctx, HPACK_EVT_VALUE, JUMP(he, tmp.nam_sz + 1),
		    tmp.val_sz);
		he = MOVE(he, off);
	}

	assert(DIFF(tbl, he) == ctx->hp->sz.len);
}

/**********************************************************************
 * Resize
 */

void
HPT_adjust(struct hpack_ctx *ctx, size_t len)
{
	struct hpack *hp;
	struct hpt_entry *he;
	struct hpt_entry tmp;
	size_t sz, lim;

	hp = ctx->hp;
	assert(hp->sz.lim <= (ssize_t)hp->sz.max ||
	    hp->sz.nxt > (ssize_t)hp->sz.max);

	if (hp->cnt == 0)
		return;

	he = hpt_dynamic(hp, hp->cnt);
	lim = HPACK_LIMIT(hp);

	while (hp->cnt > 0 && len > lim) {
		(void)memcpy(&tmp, he, HPT_HEADERSZ);
		assert(tmp.magic == HPT_ENTRY_MAGIC);
		assert(tmp.nam_sz > 0);
		sz = HPT_OVERHEAD + tmp.nam_sz + tmp.val_sz;
		len -= sz;
		hp->sz.len -= sz;
		hp->cnt--;
		he = MOVE(he, -tmp.pre_sz);
		CALLBACK(ctx, HPACK_EVT_EVICT, NULL, 0);
	}

	if (hp->cnt == 0)
		assert(hp->sz.len == 0);
	else
		assert(hp->sz.len > 0);
}

/**********************************************************************
 * Insert
 */

static unsigned
hpt_notify(struct hpt_priv *priv, enum hpack_event_e evt, const char *buf,
    size_t len)
{
	struct hpack *hp;
	size_t lim;
	char *c;

	hp = priv->ctx->hp;
	assert(hp->sz.lim <= (ssize_t) hp->sz.max);

	switch (evt) {
	case HPACK_EVT_NAME:
		assert(len > 0);

		priv->ctx->ins = HPT_HEADERSZ + 1;
		lim = HPACK_LIMIT(hp);

		if (lim <= HPT_OVERHEAD)
			break;

		assert(priv->he == hp->tbl);
		assert(priv->he->pre_sz == 0);
		assert(hp->off == 0);

		/* NB: Table entries only use HPT_HEADERSZ (30) bytes of
		 * struct hpt_entry, because header names and values are
		 * null-terminated to make things easier. However, because
		 * of struct packing rules, in order to be consistent across
		 * different CPU architectures the intended size is rather
		 * HPT_OVERHEAD (32) bytes, with the certainty that the two
		 * unused bytes are part of the padding. The purpose of this
		 * assertion is to check that.
		 */
		assert(sizeof *hp->tbl == HPT_OVERHEAD);

		/* account for the entry header + the name null byte */
		priv->he->pre_sz = HPT_HEADERSZ + 1;
		priv->nam = 1;
		break;
	case HPACK_EVT_VALUE:
		if (priv->ctx->ins < HPACK_LIMIT(hp)) {
			/* write the name null byte */
			c = MOVE(hp->tbl, priv->ctx->ins - 1);
			*c = '\0';
			/* account for the value null byte */
			priv->ctx->ins++;
		}
		priv->nam = 0;
		break;
	case HPACK_EVT_INDEX:
		assert(buf == NULL);
		assert(len == 0);
		/* write the value null byte */
		c = MOVE(hp->tbl, priv->ctx->ins - 1);
		*c = '\0';
		break;
	default:
		WRONG("Unexpected event");
	}

	if (priv->ctx->hp->magic == DECODER_MAGIC || evt == HPACK_EVT_INDEX)
		CALLBACK(priv->ctx, evt, buf, len);

	/* is there anything to copy in the table? */
	return (evt == HPACK_EVT_NAME || (buf != NULL && len > 0));
}

static unsigned
hpt_fit(struct hpt_priv *priv, size_t len)
{
	struct hpack *hp;

	hp = priv->ctx->hp;
	assert(hp->sz.lim <= (ssize_t) hp->sz.max);

	priv->ctx->ins += len;

	/* fitting the new field may require eviction */
	HPT_adjust(priv->ctx, hp->sz.len + priv->ctx->ins);

	/* does the new field even fit alone? */
	if (priv->ctx->ins > HPACK_LIMIT(hp)) {
		assert(hp->sz.len == 0);
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
	end = bgn + hp->sz.len;

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
	struct hpt_entry tmp;
	void *ptr;

	assert(buf != NULL);
	hp = priv->ctx->hp;

	if (hp->cnt > 0) {
		(void)memcpy(&tmp, priv->he, HPT_HEADERSZ);
		assert(tmp.magic == HPT_ENTRY_MAGIC);
		ptr = priv->he;
		hp->off = priv->ctx->ins;
		tmp.pre_sz = priv->ctx->ins;
		priv->he = MOVE(hp->tbl, tmp.pre_sz);
		(void)memmove(priv->he, ptr, hp->sz.len);
		(void)memcpy(priv->he, &tmp, HPT_HEADERSZ);
	}

	if (hpt_overlap(hp, buf, len))
		buf += priv->ctx->ins;

	(void)memcpy(priv->wrt, buf, len);
}

static void
hpt_copy_evicted(struct hpt_priv *priv, const char *buf, size_t len)
{
	struct hpack *hp;
	void *ptr;
	char tmp[64];
	size_t sz, mv;

	assert(priv->he->magic == HPT_ENTRY_MAGIC);

	hp = priv->ctx->hp;
	if (hp->cnt == 0) {
		hpt_copy(priv, buf, len);
		return;
	}

	assert(hp->sz.len > sizeof *priv->he);
	mv = hp->sz.len - HPT_HEADERSZ;

	while (len > 0) {
		sz = len < sizeof tmp ? len : sizeof tmp;
		ptr = MOVE(priv->wrt, sz);

		(void)memcpy(tmp, buf, sz);
		(void)memmove(ptr, priv->wrt, mv);
		(void)memcpy(priv->wrt, tmp, sz);

		buf += sz;
		len -= sz;
		priv->wrt = ptr;
	}

	assert(priv->he->magic == HPT_ENTRY_MAGIC);

	/* make room for the entry metadata and the name's null byte */
	ptr = MOVE(priv->wrt, HPT_HEADERSZ + 1);
	(void)memmove(ptr, priv->wrt, mv);

	/* update and copy the entry metadata */
	assert(priv->he == hp->tbl);
	hp->off = priv->ctx->ins;
	hp->tbl->pre_sz = priv->ctx->ins;
	priv->he = MOVE(hp->tbl, priv->he->pre_sz);
	(void)memcpy(priv->he, hp->tbl, HPT_HEADERSZ);
}

void
HPT_insert(enum hpack_event_e evt, const char *buf, size_t len, void *priv)
{
	struct hpt_priv *priv2;
	struct hpack *hp;
	unsigned ovl;
#ifndef NDEBUG
	struct hpt_entry tmp;
#endif

	assert(evt != HPACK_EVT_NEVER);
	assert(evt != HPACK_EVT_EVICT);
	assert(evt != HPACK_EVT_TABLE);
	assert(evt != HPACK_EVT_DATA);

	priv2 = priv;
	hp = priv2->ctx->hp;

	ovl = hpt_overlap(hp, buf, len);
	if (ovl) {
		assert(evt == HPACK_EVT_NAME);
		assert(hp->cnt > 0);
	}

	if (!hpt_notify(priv2, evt, buf, len) || !hpt_fit(priv2, len))
		return;

	priv2->wrt = evt == HPACK_EVT_NAME ?
	    JUMP(hp->tbl, 0) :
	    MOVE(hp->tbl, priv2->ctx->ins - len - 1);

	/* NB: If the name belonged in the dynamic table when first checked,
	 * but no longer overlaps once eviction has been performed, we end up
	 * in a situation described in RFC 7541 section 4.4. and make the more
	 * expensive and cautious copy.
	 *
	 * What the RFC says:
	 *
	 * A new entry can reference the name of an entry in the dynamic table
	 * that will be evicted when adding this new entry into the dynamic
	 * table.  Implementations are cautioned to avoid deleting the
	 * referenced name if the referenced entry is evicted from the dynamic
	 * table prior to inserting the new entry.
	 */
	if (ovl && !hpt_overlap(hp, buf, len))
		hpt_copy_evicted(priv2, buf, len);
	else
		hpt_copy(priv2, buf, len);

	if (evt == HPACK_EVT_NAME) {
		if (hp->cnt > 0) {
#ifndef NDEBUG
			(void)memcpy(&tmp, priv2->he, HPT_HEADERSZ);
			assert(priv2->he != hp->tbl);
			assert(tmp.pre_sz >= HPT_HEADERSZ);
#endif
		}
		(void)memset(hp->tbl, 0, HPT_HEADERSZ);
		hp->tbl->magic = HPT_ENTRY_MAGIC;
	}

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

	EXPECT(ctx, IDX, idx > 0);
	(void)memset(&hf, 0, sizeof hf);
	CALL(HPT_search, ctx, idx, &hf);
	assert(hf.nam != NULL);
	assert(hf.val != NULL);
	CALLBACK(ctx, HPACK_EVT_NAME, hf.nam, hf.nam_sz);
	CALLBACK(ctx, HPACK_EVT_VALUE, hf.val, hf.val_sz);

	return (0);
}

int
HPT_decode_name(HPACK_CTX)
{
	struct hpt_field hf;

	assert(ctx->hp->state.idx != 0);
	(void)memset(&hf, 0, sizeof hf);
	CALL(HPT_search, ctx, ctx->hp->state.idx, &hf);
	assert(hf.nam != NULL);
	CALLBACK(ctx, HPACK_EVT_NAME, hf.nam, hf.nam_sz);

	return (0);
}
