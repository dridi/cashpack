/*-
 * License: BSD-2-Clause
 * (c) 2020 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 */

#ifdef NDEBUG
#  undef NDEBUG
#endif

#include <sys/types.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "hpack.h"
#include "hpack_assert.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	enum hpack_result_e res;
	struct hpack_decoding dec;
	struct hpack *hp;
	const char *n, *v;
	char buf[4096];

	hp = hpack_decoder(4096, -1, hpack_default_alloc);
	assert(hp != NULL);

	dec.blk = data;
	dec.blk_len = size;
	dec.buf = buf;
	dec.buf_len = sizeof buf;
	dec.cb = NULL;
	dec.priv = NULL;
	dec.cut = 0;

	n = v = NULL;

	do {
		res = hpack_decode_fields(hp, &dec, &n, &v);
	} while (res == HPACK_RES_FLD);

	hpack_free(&hp);

	return (res < 0 ? EXIT_FAILURE : EXIT_SUCCESS);
}
