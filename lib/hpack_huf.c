/*-
 * License: BSD-2-Clause
 * (c) 2016-2017 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 *
 * HPACK Huffman Code (RFC 7541 Appendix B)
 */

#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hpack.h"
#include "hpack_priv.h"
#include "hpack_huf_dec.h"
#include "hpack_huf_enc.h"

/**********************************************************************
 * Decode
 */

static int
hph_decode_lookup(HPACK_CTX, int *eos)
{
	struct hpack_state *hs;
	uint8_t cod;

	hs = &ctx->hp->state;

	assert(hs->stt.str.blen >= 8);
	while (hs->stt.str.blen >= hs->stt.str.oct->len) {
		cod = (hs->stt.str.bits >> (16 - hs->stt.str.dec->len)) &
		    0xff;

		/* premature EOS */
		EXPECT(ctx, HUF, hs->stt.str.oct[cod].len > 0);

		if (hs->stt.str.blen < hs->stt.str.oct[cod].len)
			break; /* more bits needed */

		*eos = 1;
		hs->stt.str.dec = hs->stt.str.oct[cod].nxt;
		if (hs->stt.str.dec == NULL) {
			CALL(HPD_putc, ctx, hs->stt.str.oct[cod].chr);
			hs->stt.str.dec = &hph_dec0;
			*eos = 0;
		}

		hs->stt.str.blen -= hs->stt.str.oct[cod].len;
		hs->stt.str.bits <<= hs->stt.str.oct[cod].len;
		hs->stt.str.oct = hs->stt.str.dec->oct;
	}
	return (0);
}

int
HPH_decode(HPACK_CTX, size_t len)
{
	struct hpack_state *hs;
	int eos;

	hs = &ctx->hp->state;
	eos = 0;

	if (hs->stt.str.dec == NULL) {
		hs->stt.str.dec = &hph_dec0;
		hs->stt.str.oct = hph_oct0;
	}

	if (len > ctx->ptr_len)
		len = ctx->ptr_len;

	while (hs->stt.str.len > 0) {
		EXPECT(ctx, BUF, len > 0);
		assert(hs->stt.str.blen < 8);
		hs->stt.str.bits |= *ctx->ptr.blk << (8 - hs->stt.str.blen);
		hs->stt.str.blen += 8;
		hs->stt.str.len--;
		ctx->ptr.blk++;
		ctx->ptr_len--;
		len--;

		CALL(hph_decode_lookup, ctx, &eos);
	}

	EXPECT(ctx, HUF, eos == 0); /* spurious EOS */

	if (hs->stt.str.blen > 0) {
		/* check padding */
		assert(hs->stt.str.blen < 8);
		EXPECT(ctx, HUF, hs->stt.str.bits ==
		    (uint16_t)(0xffff << (16 - hs->stt.str.blen)));
	}
	else {
		/* no padding */
		assert(hs->stt.str.bits == 0);
	}

	CALL(HPD_putc, ctx, '\0');

	return (0);
}

void
HPH_encode(HPACK_CTX, const char *str)
{
	uint64_t bits;
	size_t sz;
	uint8_t c;

	bits = 0;
	sz = 0;

	while (*str != '\0') {
		c = (uint8_t)*str;
		bits = (bits << hph_enc[c].len) | hph_enc[c].cod;
		sz += hph_enc[c].len;

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

size_t
HPH_size(const char *str)
{
	size_t sz;

	assert(str != NULL);

	sz = 7;

	while (*str != '\0') {
		sz += hph_enc[(uint8_t)*str].len;
		str++;
	}

	return (sz >> 3);
}
