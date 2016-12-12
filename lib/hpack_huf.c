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
 * HPACK Huffman Code (RFC 7541 Appendix B)
 */

#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hpack.h"
#include "hpack_priv.h"
#include "hpack_huf_idx.h"
#include "hpack_huf_tbl.h"
#include "hpack_huf_dec.h"

/**********************************************************************
 * Decode
 */

static int
hph_decode_lookup(HPACK_CTX, int *eos)
{
	struct hpack_state *hs;
	uint8_t cod;

	hs = &ctx->hp->state;

	assert(hs->blen >= 8);
	while (hs->blen >= hs->oct->len) {
		cod = hs->bits >> (16 - hs->dec->len);
		EXPECT(ctx, HUF, hs->oct[cod].len > 0); /* premature EOS */

		if (hs->blen < hs->oct[cod].len)
			break; /* more bits needed */

		*eos = 1;
		hs->dec = hs->oct[cod].nxt;
		if (hs->dec == NULL) {
			CALL(HPD_putc, ctx, hs->oct[cod].chr);
			hs->dec = &hph_dec0;
			*eos = 0;
		}

		hs->blen -= hs->oct[cod].len;
		hs->bits <<= hs->oct[cod].len;
		hs->oct = hs->dec->oct;
	}
	return (0);
}

int
HPH_decode(HPACK_CTX, enum hpack_event_e evt, size_t len)
{
	struct hpack_state *hs;
	hpack_validate_f *val;
	char *ptr;
	int eos, l;

	hs = &ctx->hp->state;
	ptr = ctx->buf;
	eos = 0;

	if (hs->first) {
		assert(len == hs->len);
		hs->dec = &hph_dec0;
		hs->oct = hph_oct0;
		hs->blen = 0;
		hs->bits = 0;
		hs->first = 0;
	}

	if (len > ctx->len)
		len = ctx->len;

	while (hs->len > 0) {
		EXPECT(ctx, BUF, len > 0);
		assert(hs->blen < 8);
		hs->bits |= *ctx->blk << (8 - hs->blen);
		hs->blen += 8;
		ctx->blk++;
		ctx->len--;
		hs->len--;
		len--;

		CALL(hph_decode_lookup, ctx, &eos);
	}

	EXPECT(ctx, HUF, eos == 0); /* spurious EOS */

	assert(ctx->buf >= ptr);
	l = ctx->buf - ptr;
	if (l > 0) {
		val = evt == HPACK_EVT_NAME ? HPV_token : HPV_value;
		CALL(val, ctx, ptr, l, 1);
	}

	if (hs->blen > 0) {
		/* check padding */
		assert(hs->blen < 8);
		EXPECT(ctx, HUF,
		    hs->bits == (uint16_t)(0xffff << (16 - hs->blen)));
	}
	else {
		/* no padding */
		assert(hs->bits == 0);
	}

	CALL(HPD_putc, ctx, '\0');

	return (0);
}

void
HPH_encode(HPACK_CTX, const char *str)
{
	uint64_t bits;
	size_t sz, i;

	bits = 0;
	sz = 0;

	while (*str != '\0') {
		i = hph_idx[(uint8_t)*str];
		bits = (bits << hph_tbl[i].len) | hph_tbl[i].cod;
		sz += hph_tbl[i].len;

		while (sz >= 8) {
			sz -= 8;
			HPE_putb(ctx, (uint8_t)(bits >> sz));
		}

		str++;
	}

	assert(sz < 8);
	if (sz > 0) {
		sz = 8 - sz; /* padding bits */
		bits <<= sz;
		bits |= (1 << sz) - 1;
		HPE_putb(ctx, (uint8_t)bits);
	}
}

void
HPH_size(const char *str, size_t *res)
{
	uint16_t i;
	size_t sz;

	sz = 0;

	while (*str != '\0') {
		i = hph_idx[(uint16_t)*str];
		sz += hph_tbl[i].len;
		str++;
	}

	*res = sz >> 3;
	if (sz & 0x07)
		*res += 1;
}
