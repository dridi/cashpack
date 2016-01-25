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
#include "hpack_huf_tbl.h"

/**********************************************************************
 * Decode
 */

int
HPH_decode(HPACK_CTX, hpack_validate_f val, size_t len)
{
	const struct hph_entry *he;
	uint64_t bits;
	uint32_t cod;
	uint16_t blen;
	char buf[256];
	unsigned eos, l, first;

	bits = 0;
	blen = 0;
	l = 0;
	first = 1;

	while (len > 0 || blen > 0) {
		he = &hph_tbl[9]; /* the last 5-bit code */
		cod = UINT16_MAX;
		eos = 1;

		while (he != hph_tbl) {
			if (blen < he->len) {
				if (len == 0)
					break;
				bits = (bits << 8) | *ctx->buf;
				blen += 8;
				ctx->buf++;
				ctx->len--;
				len--;
			}
			cod = bits >> (blen - he->len);
			if (cod <= he->cod) {
				eos = 0;
				break;
			}
			he = &hph_tbl[he->nxt];
		}

		if (eos)
			break;

		assert(he->cod >= cod);
		he -= he->cod - cod;
		assert(he->cod == cod);

		assert(l < sizeof buf);
		buf[l] = he->chr;
		if (++l == sizeof buf) {
			CALL(val, ctx, (char *)buf, l, first);
			CALLBACK(ctx, HPACK_EVT_DATA, buf, l);
			l = 0;
			first = 0;
		}

		assert(blen >= he->len);
		blen -= he->len;
		bits &= (1 << blen) - 1;
	}

	EXPECT(ctx, HUF, len == 0); /* premature EOS */

	if (eos) {
		/* check padding */
		assert(blen > 0);
		EXPECT(ctx, HUF, blen < 8);
		EXPECT(ctx, HUF, bits + 1 == (uint64_t)(1 << blen));
	}
	else {
		/* no padding */
		assert(bits == 0);
		assert(blen == 0);
	}

	if (l > 0) {
		CALL(val, ctx, (char *)buf, l, first);
		CALLBACK(ctx, HPACK_EVT_DATA, buf, l);
	}

	return (0);
}
