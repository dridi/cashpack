/*-
 * License: BSD-2-Clause
 * (c) 2016 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "gen.h"

#define TBL_LEN 256

struct hph {
	uint32_t	cod;
	uint16_t	len;
	uint8_t		chr;
};

static const struct hph tbl[] = {
#define HPH(c, h, l) { h, l, c },
#include "tbl/hpack_huffman.h"
#undef HPH
};

int
main(void)
{
	int i, j;

	GEN_HDR();
	GEN("#define HPH_TBL_LEN %d", TBL_LEN);
	OUT("");
	OUT("struct hph_enc {");
	OUT("\tuint32_t\tcod;");
	OUT("\tuint16_t\tlen;");
	OUT("};");
	OUT("");
	OUT("static const struct hph_enc hph_enc[] = {");
	for (i = 0; i < TBL_LEN; i++) {
		for (j = 0; tbl[j].chr != i; j++)
			assert(j < TBL_LEN);
		GEN("\t{0x%08x, %2d},", tbl[j].cod, tbl[j].len);
	}
	OUT("};");

	return (0);
}
