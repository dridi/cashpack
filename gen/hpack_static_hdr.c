/*-
 * License: BSD-2-Clause
 * (c) 2017-2020 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
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
	int cmp;

	h1 = v1;
	h2 = v2;

	cmp = (int)(strlen(h1->nam) - strlen(h2->nam));
	if (cmp)
		return (cmp);

	cmp = strcmp(h1->nam, h2->nam);
	if (cmp)
		return (cmp);

	cmp = (int)(strlen(h1->val) - strlen(h2->val));
	if (cmp)
		return (cmp);
	return (strcmp(h1->val, h2->val));
}

int
main(void)
{
	const struct hdr *fld;

	qsort(static_tbl, HDR_LEN, sizeof static_tbl[0], hdr_cmp);

	GEN_HDR();
	OUT("static const struct hpt_field hpack_static_hdr[] = {");
	fld = static_tbl;
	while (fld->nam != NULL) {
		GEN("\t{ \"%s\", \"%s\", %zu, %zu, %zu },",
		    fld->nam, fld->val, strlen(fld->nam), strlen(fld->val),
		    fld->idx);
		fld++;
	}
	OUT("};");

	return (0);
}
