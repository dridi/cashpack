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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hpack.h"
#include "hpack_priv.h"

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
 */

void
HPT_insert(void *priv, enum hpack_evt_e evt, const void *buf, size_t len)
{
	struct hpt_priv *priv2;
	struct hpack *hp;
	uintptr_t off;

	assert(evt != HPACK_EVT_NEVER);
	assert(evt != HPACK_EVT_INDEX);
	assert(evt != HPACK_EVT_TABLE);

	priv2 = priv;
	hp = priv2->ctx->hp;

	/* XXX: eviction */

	switch (evt) {
	case HPACK_EVT_FIELD:
		assert(len == 32); /* entry overhead, see Section 4.1 */
		CALLBACK(priv2->ctx, evt, NULL, 0);
		break;
	case HPACK_EVT_NAME:
	case HPACK_EVT_VALUE:
	case HPACK_EVT_DATA:
		CALLBACK(priv2->ctx, evt, buf, len);
		break;
	default:
		WRONG("Unexpected event");
	}

	off = (uintptr_t)hp->tbl + priv2->len;
	priv2->len += len;
	hp->len += len;
	if (priv2->len > hp->lim) {
		hp->len = 0;
		return;
	}

	switch (evt) {
	case HPACK_EVT_FIELD:
		memset(hp->tbl, 0, sizeof *hp->tbl);
		return;
	case HPACK_EVT_NAME:
		hp->tbl[0].nam_sz += len;
		break;
	case HPACK_EVT_VALUE:
		hp->tbl[0].val_sz += len;
		break;
	case HPACK_EVT_DATA:
		INCOMPL();
		break;
	default:
		WRONG("Unexpected event");
	}

	memcpy((void *)off, buf, len);
}

/**********************************************************************
 */

int
HPT_search(HPACK_CTX, size_t idx, struct hpt_field *hf)
{
	const struct hpack *hp;
	const struct hpt_entry *he;
	size_t off;

	if (idx <= HPT_STATIC_MAX) {
		memcpy(hf, &hpt_static[idx - 1], sizeof *hf);
		return (0);
	}

	hp = ctx->hp;
	idx -= HPT_STATIC_MAX;
	EXPECT(ctx, IDX, idx <= hp->cnt);
	off = 0;
	he = hp->tbl;

	while (1) {
		assert(DIFF(hp->tbl, he) < hp->len);
		assert(he->pre_sz == off);
		assert(he->nam_sz > 0);
		assert(he->val_sz > 0);
		off = sizeof *he + he->nam_sz + he->val_sz;
		if (--idx == 0)
			break;
		he = JUMP(he, he->nam_sz + he->val_sz);
	}

	hf->nam_sz = he->nam_sz;
	hf->val_sz = he->val_sz;
	hf->nam = JUMP(he, 0);
	hf->val = JUMP(he, he->nam_sz);
	return (0);
}

int
HPT_decode(HPACK_CTX, size_t idx)
{
	struct hpt_field hf;

	assert(idx != 0);
	memset(&hf, 0, sizeof hf);
	CALL(HPT_search, ctx, idx, &hf);
	EXPECT(ctx, IDX, hf.nam != NULL);
	EXPECT(ctx, IDX, hf.val != NULL);
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
	EXPECT(ctx, IDX, hf.nam != NULL);
	CALLBACK(ctx, HPACK_EVT_NAME, hf.nam, hf.nam_sz);

	return (0);
}
