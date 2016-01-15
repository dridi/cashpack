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
#include "hpack_priv.h"

#define WRT(buf, len)				\
	do {					\
		write(STDOUT_FILENO, buf, len);	\
	} while (0)

#define OUT(str)	WRT(str, sizeof(str) - 1)

void
print_headers(void *priv, enum hpack_evt_e evt, const void *buf, size_t len)
{

	assert(priv == NULL);

	switch (evt) {
	case HPACK_EVT_FIELD:
		assert(buf == NULL);
		OUT("\n");
		break;
	case HPACK_EVT_NEVER:
		assert(buf == NULL);
		assert(len == 0);
		break;
	case HPACK_EVT_NAME:
		assert(buf != NULL);
		assert(len > 0);
		WRT(buf, len);
		OUT(": ");
		break;
	case HPACK_EVT_VALUE:
		assert(buf != NULL);
		assert(len > 0);
		WRT(buf, len);
		break;
	default:
		fprintf(stderr, "Unknwon event: %d\n", evt);
		abort();
	}
}

void
print_entries(struct hpack *hp)
{
	struct hpack_ctx ctx;
	struct hpt_field hf;
	char buf[sizeof "\n[  1] (s =  55) "];
	size_t i, retval, l, s;

	memset(&ctx, 0, sizeof ctx);
	ctx.hp = hp;

	for (i = 1; i <= hp->cnt; i++) {
		memset(&hf, 0, sizeof hf);
		retval = HPT_search(&ctx, i + HPT_STATIC_MAX, &hf);

		assert(retval == 0);
		assert(hf.nam_sz > 0);
		assert(hf.val_sz > 0);
		assert(hf.nam != NULL);
		assert(hf.val != NULL);

		s = 32 + hf.nam_sz + hf.val_sz;
		l = snprintf(buf, sizeof buf, "\n[%3lu] (s = %3lu) ", i, s);
		assert(l + 1 == sizeof  buf);
		WRT(buf, sizeof(buf) - 1);

		WRT(hf.nam, hf.nam_sz);
		OUT(": ");
		WRT(hf.val, hf.val_sz);
	}

	OUT("\n      Table size: ");
	l = snprintf(buf, sizeof buf, "%3lu", hp->len);
	assert(l == 3);
	WRT(buf, l);
	OUT("\n");
}

int
main(int argc, char **argv)
{
	enum hpack_res_e res;
	struct hpack *hp;
	struct stat st;
	void *buf;
	int fd, tbl_sz;

	tbl_sz = 0;

	/* ignore the command name */
	argc--;
	argv++;

	/* handle options */
	if (!strcmp("-t", *argv)) {
		assert(argc > 2);
		tbl_sz = atoi(argv[1]);
		assert(tbl_sz > 0);
		argc -= 2;
		argv += 2;
	}

	/* exactly one file name is expected */
	assert(argc == 1);

	fd = open(*argv, O_RDONLY);
	assert(fd > STDERR_FILENO);

	assert(fstat(fd, &st) == 0);

	buf = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	assert(buf != NULL);

	hp = HPACK_decoder(tbl_sz);
	assert(hp != NULL);
	hp->lim = tbl_sz; /* XXX: what is the initial limit? */

	OUT("Decoded header list:\n");

	res = HPACK_decode(hp, buf, st.st_size, print_headers, NULL);
	assert(res == HPACK_RES_OK);

	OUT("\n\nDynamic table (after decoding):");
	if (hp->cnt == 0) {
		OUT(" empty.\n");
		assert(hp->len == 0);
	}
	else {
		OUT("\n");
		assert(hp->len > 0);
		assert(hp->len <= hp->lim);
		assert(hp->lim <= hp->max);
		print_entries(hp);
	}

	HPACK_free(&hp);

	return (0);
}
