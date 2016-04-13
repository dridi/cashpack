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
 */

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "hpack.h"
#include "hpack_assert.h"

#include "tst.h"

struct tst_ctx {
	size_t	cnt;
	size_t	len;
	char	buf[8];
	size_t	sz;
};

void
print_nothing(void *priv, enum hpack_evt_e evt, const char *buf, size_t len)
{

	assert(priv == NULL);
	(void)priv;
	(void)evt;
	(void)buf;
	(void)len;
}

void
print_headers(void *priv, enum hpack_evt_e evt, const char *buf, size_t len)
{

	assert(priv == NULL);
	(void)priv;

	switch (evt) {
	case HPACK_EVT_FIELD:
		assert(buf == NULL);
		OUT("\n");
		break;
	case HPACK_EVT_INDEX:
	case HPACK_EVT_NEVER:
	case HPACK_EVT_EVICT:
		assert(buf == NULL);
		assert(len == 0);
		break;
	case HPACK_EVT_VALUE:
		OUT(": ");
		/* fall through */
	case HPACK_EVT_NAME:
		if (buf != NULL)
			WRT(buf, len);
		break;
	case HPACK_EVT_DATA:
		assert(buf != NULL);
		assert(len > 0);
		WRT(buf, len);
		break;
	default:
		WRONG("Unknown event");
	}
}

void
print_entries(void *priv, enum hpack_evt_e evt, const char *buf, size_t len)
{
	struct tst_ctx *ctx;
	char str[sizeof "\n[IDX] (s = LEN) "];
	int l;

	assert(priv != NULL);
	ctx = priv;
	if (ctx->cnt == 0)
		OUT("\n");

	switch (evt) {
	case HPACK_EVT_FIELD:
		assert(buf == NULL);
		assert(len > 0);
		ctx->cnt++;
		ctx->len += len;
		l = snprintf(str, sizeof str, "\n[%3zu] (s = %3zu) ",
		    ctx->cnt, len);
		assert(l + 1 == sizeof  str);
		WRT(str, l);
		break;
	case HPACK_EVT_VALUE:
		OUT(": ");
		/* fall through */
	case HPACK_EVT_NAME:
		assert(buf != NULL);
		WRT(buf, len);
		break;
	default:
		WRONG("Unexpected event");
	}
}

int
main(int argc, char **argv)
{
	enum hpack_res_e res, exp;
	hpack_decoded_f *cb;
	struct hpack *hp;
	struct stat st;
	struct tst_ctx ctx;
	void *buf;
	int fd, retval, tbl_sz;

	tbl_sz = 4096; /* RFC 7540 Section 6.5.2 */
	exp = HPACK_RES_OK;
	cb = print_headers;

	/* ignore the command name */
	argc--;
	argv++;

	/* handle options */
	if (argc > 0 && !strcmp("-r", *argv)) {
		assert(argc > 2);
#define HPR_ERRORS_ONLY
#define HPR(val, cod, txt)			\
		if (!strcmp(argv[1], #val))	\
			exp = HPACK_RES_##val;
#include "tbl/hpack_tbl.h"
#undef HPR
#undef HPR_ERRORS_ONLY
		assert(exp != HPACK_RES_OK);
		cb = print_nothing;
		argc -= 2;
		argv += 2;
	}

	if (argc > 0 && !strcmp("-t", *argv)) {
		assert(argc > 2);
		tbl_sz = atoi(argv[1]);
		assert(tbl_sz > 0);
		argc -= 2;
		argv += 2;
	}

	/* exactly one file name is expected */
	if (argc != 1) {
		fprintf(stderr, "Usage: hdecode [-r <expected result>] "
		    "[-t <table size>] <dump file>\n\n"
		    "The file contains a dump of HPACK octets.\n"
		    "Default table size: 4096\n"
		    "Possible results:\n");

#define HPR_ERRORS_ONLY
#define HPR(val, cod, txt) fprintf(stderr, "  %s: %s\n", #val, txt);
#include "tbl/hpack_tbl.h"
#undef HPR
#undef HPR_ERRORS_ONLY

		return (EXIT_FAILURE);
	}

	fd = open(*argv, O_RDONLY);
	assert(fd > STDERR_FILENO);

	retval = fstat(fd, &st);
	assert(retval == 0);
	(void)retval;

	buf = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	assert(buf != MAP_FAILED);

	hp = hpack_decoder(tbl_sz, hpack_default_alloc);
	assert(hp != NULL);

	OUT("Decoded header list:\n");

	res = hpack_decode(hp, buf, st.st_size, cb, NULL);

	OUT("\n\nDynamic Table (after decoding):");
	ctx.cnt = 0;
	ctx.len = 0;
	hpack_foreach(hp, print_entries, &ctx);
	if (ctx.cnt == 0) {
		assert(ctx.len == 0);
		OUT(" empty.\n");
	}
	else {
		assert(ctx.len > 0);
		ctx.sz = snprintf(ctx.buf, sizeof ctx.buf, "%3zu\n", ctx.len);
		OUT("\n      Table size: ");
		WRT(ctx.buf, ctx.sz);
	}

	hpack_free(&hp);

	retval = munmap(buf, st.st_size);
	assert(retval == 0);

	retval = close(fd);
	assert(retval == 0);

	return (res != exp);
}
