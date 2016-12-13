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
main(int argc, const char **argv)
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

	(void)argc;
	(void)argv;
	return (0);
}
