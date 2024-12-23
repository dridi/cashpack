/*-
 * License: BSD-2-Clause
 * (c) 2017 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "hpack.h"
#include "hpack_priv.h"

void
HPC_notify(HPACK_CTX, enum hpack_event_e evt, const void *buf, size_t len)
{
	if (ctx->flg & HPACK_CTX_TOO_BIG)
		assert(ctx->hp->magic == DECODER_MAGIC);
	else
		ctx->cb(evt, buf, len, ctx->priv);
}
