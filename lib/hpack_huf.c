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

#define MASK(pack, n) (pack >> (64 - n))

#include <stdio.h>
int
HPH_decode(HPACK_CTX, hpack_validate_f val, size_t len)
{
	char buf[256];
	uint64_t pack = 0;
	int pl = 0; /* pack length*/
	unsigned first =1, l =0;
	struct stbl *tbl = &byte0;
	struct ssym *sym;

	while (len > 0 || pl != 0) {
		assert(pl >= 0);
		/* make sure we have enough data*/
		if (pl < tbl->msk) {
			if (MASK(pack, pl) == (unsigned)((1 << pl) - 1) &&
					len == 0){
					/* flush what we have */
					if (l > 0) {
						CALL(val, ctx, buf, l, first);
						CALLBACK(ctx, HPACK_EVT_DATA,
								buf, l);
					}
					EXPECT(ctx, HUF, tbl == &byte0);
					return (0);
			}
			/* fit as many bytes as we can in pack */
			while (pl <= 56 && len > 0) {
				pack |= (uint64_t)(*ctx->buf & 0xff)
							<< (56 - pl);
				pl += 8;
				ctx->buf++;
				ctx->len--;
				len--;
			}
		}

		assert(tbl);
		assert(tbl->msk);
		sym = &tbl->syms[MASK(pack, tbl->msk)];

		assert(sym->csm <= tbl->msk);

		/* nothing consumed, it's an error */
		EXPECT(ctx, HUF, sym->csm > 0);
		EXPECT(ctx, HUF, pl >= sym->csm);

		pack <<= sym->csm;
		pl -= sym->csm;
		/* point to the next table */
		if (sym->nxt) {
			tbl = sym->nxt;
			continue;
		}
		buf[l] = sym->chr;
		tbl = &byte0;
		if (++l == sizeof buf) {
			CALL(val, ctx, (char *)buf, l, first);
			CALLBACK(ctx, HPACK_EVT_DATA, buf, l);
			l = 0;
			first = 0;
		}
	}

	if (l > 0) {
		CALL(val, ctx, (char *)buf, l, first);
		CALLBACK(ctx, HPACK_EVT_DATA, buf, l);
	}
	/* are we expecting more bits? */
	EXPECT(ctx, HUF, tbl == &byte0);
	return (0);
}

void
HPH_encode(HPACK_CTX, const char *str)
{
	uint64_t bits;
	uint8_t buf[256], *cur;
	size_t sz, len, i;

	bits = 0;
	cur = buf;
	sz = 0;
	len = 0;

	while (*str != '\0') {
		i = hph_idx[(uint8_t)*str];
		bits = (bits << hph_tbl[i].len) | hph_tbl[i].cod;
		sz += hph_tbl[i].len;

		while (sz >= 8) {
			sz -= 8;
			*cur = (uint8_t)(bits >> sz);
			cur++;
			if (++len == sizeof buf) {
				HPE_push(ctx, buf, sizeof buf);
				cur = buf;
				len = 0;
			}
		}

		str++;
	}

	assert(sz < 8);
	if (sz > 0) {
		sz = 8 - sz; /* padding bits */
		bits <<= sz;
		bits |= (1 << sz) - 1;
		*cur = (uint8_t)bits;
		len++;
	}

	HPE_push(ctx, buf, len);
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
