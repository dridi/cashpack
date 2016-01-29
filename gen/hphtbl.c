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
#include <string.h>

#include "gen.h"

#define TBL_LEN 256
#define TBL_FMT "\t{ 0x%08x, %3d, 0x%02x, %3d },"

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
gen(int i, int j, int k)
{
	if (j > 0)
		GEN(TBL_FMT, tbl[j].cod, tbl[j].len, tbl[j].chr, i - 1);
	for (j++; j < k; j++)
		GEN(TBL_FMT, tbl[j].cod, tbl[j].len, tbl[j].chr, 0);
	return (j);
}

int
main(int argc, const char **argv)
{
	uint16_t l;
	int i, j;

	GEN_HDR();
	GEN("#define HPH_TBL_LEN %d", TBL_LEN);
	OUT("");
	OUT("struct hph_entry {");
	OUT("\tuint32_t\tcod;");
	OUT("\tuint16_t\tlen;");
	OUT("\tchar\t\tchr;");
	OUT("\tuint8_t\t\tnxt;");
	OUT("};");
	OUT("");
	OUT("static const struct hph_entry hph_tbl[] = {");
	j = -1;
	l = tbl[0].len;
	for (i = 0; i < TBL_LEN; i++) {
		assert(l <= tbl[i].len);
		if (l < tbl[i].len) {
			assert(j < i - 1);
			j = gen(i, j, i - 1);
			l = tbl[i].len;
		}
	}
	(void)gen(i, j, TBL_LEN);
	OUT("};");

	(void)argc;
	(void)argv;
	return (0);
}
