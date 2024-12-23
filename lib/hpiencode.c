/*-
 * License: BSD-2-Clause
 * (c) 2016-2017 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 */

#undef NDEBUG

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "hpack.h"
#include "hpack_assert.h"
#include "hpack_priv.h"

int
main(int argc, const char **argv)
{
	struct hpack_encoding enc;
	struct hpack_ctx ctx;
	enum hpi_prefix_e pfx;
	enum hpi_pattern_e pat;
	int val, ok;
	uint8_t buf[8];

	/* ignore the program name */
	argc--;
	assert(argc == 2);

	ok = 0;
	pfx = (enum hpi_prefix_e)-1;
	pat = (enum hpi_pattern_e)-1;

#define HPP(nm, px, pt)				\
	if (!strcasecmp(#nm, argv[1])) {	\
		pfx = px;			\
		pat = pt;			\
		ok = 1;				\
	}
#include "tbl/hpack_tbl.h"
#undef HPP

	if (!ok)
		WRONG("Unknown prefix");

	val = atoi(argv[2]);

	(void)memset(&enc, 0, sizeof enc);
	enc.buf = buf;
	enc.buf_len = sizeof buf;

	(void)memset(&ctx, 0, sizeof ctx);
	ctx.arg.enc = &enc;
	ctx.ptr.cur = buf;

	HPI_encode(&ctx, pfx, pat, (uint16_t)val);

	assert(ctx.ptr_len > 0);

	while (ctx.ptr_len > 0) {
		printf("%02x", *(uint8_t *)enc.buf);
		enc.buf = (uint8_t *)enc.buf + 1;
		ctx.ptr_len--;
	}
	puts("");

	return (0);
}
