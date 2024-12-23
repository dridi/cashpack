/*-
 * License: BSD-2-Clause
 * (c) 2016-2019 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
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
#include <unistd.h>

#include "hpack.h"
#include "hpack_assert.h"
#include "hpack_priv.h"

int
HPI_decode(HPACK_CTX, enum hpi_prefix_e pfx, uint16_t *val)
{
	struct hpack_state *hs;
	uint16_t n;
	uint8_t b, mask;

	assert(pfx >= 4 && pfx <= 7);
	assert(val != NULL);

	hs = &ctx->hp->state;
	if (hs->bsy)
		assert(hs->magic == INT_STATE_MAGIC);
	else
		hs->magic = INT_STATE_MAGIC;

	hs->stt.hpi.l = 0;

	assert(ctx->ptr_len > 0);
	if (!hs->bsy) {
		mask = (uint8_t)((1 << pfx) - 1);
		hs->stt.hpi.v = *ctx->ptr.blk & mask;
		hs->stt.hpi.l++;
		ctx->ptr.blk++;
		ctx->ptr_len--;

		if (hs->stt.hpi.v < mask) {
			*val = hs->stt.hpi.v;
			return (0);
		}
		hs->stt.hpi.m = 0;
		hs->bsy = 1;
	}

	do {
		EXPECT(ctx, BUF, ctx->ptr_len > 0);
		b = *ctx->ptr.blk;
		n = hs->stt.hpi.v;
		if (hs->stt.hpi.m <= 16)
			n += (b & 0x7f) << hs->stt.hpi.m;
		else
			EXPECT(ctx, INT, (b & 0x7f) == 0);
		EXPECT(ctx, INT, hs->stt.hpi.v <= n);
		hs->stt.hpi.v = n;
		hs->stt.hpi.m += 7;
		hs->stt.hpi.l++;
		ctx->ptr.blk++;
		ctx->ptr_len--;
	} while (b & 0x80);

	*val = hs->stt.hpi.v;
	hs->bsy = 0;
	return (0);
}

void
HPI_encode(HPACK_CTX, enum hpi_prefix_e pfx, enum hpi_pattern_e pat,
    uint16_t val)
{
	uint8_t mask;

	assert(pfx >= 4 && pfx <= 7);
	assert(ctx->ptr_len < ctx->arg.enc->buf_len);

	mask = (uint8_t)((1 << pfx) - 1);
	if (val < mask) {
		HPE_putb(ctx, (uint8_t)(pat | val));
		return;
	}

	HPE_putb(ctx, (uint8_t)pat | mask);
	val -= mask;
	while (val >= 0x80) {
		HPE_putb(ctx, 0x80 | (val & 0x7f));
		val >>= 7;
	}

	HPE_putb(ctx, (uint8_t)val);
}
