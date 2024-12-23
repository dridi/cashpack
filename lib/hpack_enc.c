/*-
 * License: BSD-2-Clause
 * (c) 2016-2017 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 *
 * HPACK encoding.
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hpack.h"
#include "hpack_priv.h"

inline void
HPE_putb(HPACK_CTX, uint8_t b)
{

	assert(ctx->ptr_len < ctx->arg.enc->buf_len);

	*ctx->ptr.cur = b;
	ctx->ptr.cur++;
	ctx->ptr_len++;

	if (ctx->ptr_len == ctx->arg.enc->buf_len)
		HPE_send(ctx);
}

void
HPE_bcat(HPACK_CTX, const void *buf, size_t len)
{
	size_t sz;

	assert(buf != NULL);

	while (len > 0) {
		assert(ctx->arg.enc->buf_len > ctx->ptr_len);
		sz = ctx->arg.enc->buf_len - ctx->ptr_len;
		if (sz > len)
			sz = len;

		(void)memcpy(ctx->ptr.cur, buf, sz);
		ctx->ptr.cur += sz;
		ctx->ptr_len += sz;
		len -= sz;

		if (ctx->ptr_len == ctx->arg.enc->buf_len)
			HPE_send(ctx);
	}
}

void
HPE_send(HPACK_CTX)
{

	if (ctx->ptr_len == 0)
		return;

	HPC_notify(ctx, HPACK_EVT_DATA, ctx->arg.enc->buf, ctx->ptr_len);
	ctx->ptr.cur = ctx->arg.enc->buf;
	ctx->ptr_len = 0;
}
