/*-
 * Copyright (c) 2017 Dridi Boukelmoune
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gen.h"

#define HDR_LEN 61

struct hdr {
	const char	*nam;
	const char	*val;
	size_t		idx;
};

static struct hdr static_tbl[HDR_LEN + 1] = {
#define HPS(i, n, v)				\
	{					\
		.nam = n,			\
		.val = v,			\
		.idx = i,			\
	},
#include "tbl/hpack_static.h"
#undef HPS
	{ NULL, NULL, 0 }
};

static int
hdr_cmp(const void *v1, const void *v2)
{
	const struct hdr *h1, *h2;

	/* NB: there is no need to also compare values in this case because
	 * only pseudo-headers may collide on names in the static table.
	 */
	h1 = v1;
	h2 = v2;
	return (strcmp(h1->nam, h2->nam));
}

int
main(void)
{
	const struct hdr *fld;
	unsigned n;

	qsort(static_tbl, HDR_LEN, sizeof static_tbl[0], hdr_cmp);

	n = 0;
	fld = static_tbl;
	while (*fld->nam == ':') {
		fld++;
		n++;
	}

	GEN_HDR();
	GEN("#define HPS_PSEUDO %u", n);
	GEN("#define HPS_NORMAL %u", HDR_LEN - n);
	OUT("");
	OUT("static const struct hpt_field hpack_static_hdr[] = {");
	while (fld->nam != NULL) {
		GEN("\t{ \"%s\", \"%s\", %zu, %zu, %zu },",
		    fld->nam, fld->val, strlen(fld->nam), strlen(fld->val),
		    fld->idx);
		fld++;
	}
	OUT("};");

	return (0);
}
