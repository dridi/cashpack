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
 * HPACK Integer representation (RFC 7541 Section 5.1)
 *
 * The implementation uses 16-bit unsigned values because 255 (2^8 - 1) octets
 * would have been too little for literal header values (mostly for URLs and
 * cookies) and 65535 (2^16 - 1) octets (65KB!!) seem overkill enough.
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "hpack.h"
#include "hpack_assert.h"
#include "hpack_priv.h"

int
HPI_decode(HPACK_CTX, size_t pfx, uint16_t *val)
{
	uint16_t v, n;
	uint8_t b, m, mask;

	assert(pfx >= 4 && pfx <= 7);
	assert(val != NULL);

	EXPECT(ctx, BUF, ctx->len > 0);
	mask = (1 << pfx) - 1;
	v = *ctx->buf & mask;
	ctx->buf++;
	ctx->len--;

	if (v < mask) {
		*val = v;
		return (0);
	}

	m = 0;

	do {
		EXPECT(ctx, BUF, ctx->len > 0);
		EXPECT(ctx, INT, m < 32);
		b = *ctx->buf;
		n = v + (b & 0x7f) * (1 << m);
		EXPECT(ctx, INT, v <= n);
		v = n;
		m += 7;
		ctx->buf++;
		ctx->len--;
	} while (b & 0x80);

	*val = v;
	return (0);
}

void
HPI_encode(HPACK_CTX, size_t pfx, uint8_t pat, uint16_t val)
{
	uint8_t mask, buf[4];
	size_t i;

	buf[0] = pat;

	assert(pfx >= 4 && pfx <= 7);
	assert(ctx->len < ctx->max);

	mask = (1 << pfx) - 1;
	if (val < mask) {
		buf[0] |= (uint8_t)val;
		HPE_push(ctx, buf, 1);;
		return;
	}

	buf[0] |= mask;
	val -= mask;
	i = 1;
	while (val >= 0x80) {
		assert(i < sizeof buf);
		buf[i] = 0x80 | (val & 0x7f);
		val >>= 7;
		i++;
	}

	assert(i < sizeof buf);
	buf[i] = val;
	i++;

	HPE_push(ctx, buf, i);
}
