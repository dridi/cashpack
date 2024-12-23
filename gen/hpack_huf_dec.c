/*-
 * License: BSD-2-Clause
 * (c) 2016-2017 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "gen.h"

struct hph {
	uint32_t	cod; /* the original huffman code */
	uint32_t	msb; /* code bits aligned left */
	uint16_t	len; /* number of bits in the code */
	uint8_t		chr;
};

struct hph_dec {
	uint8_t		len;
	uint8_t		chr;
	char		nxt[sizeof "&hph_octX_pfxYYYY"];
};

static const struct hph tbl[] = {
#define HPH(c, h, l) { h, (uint32_t)h << (32 - l), l, c },
#include "tbl/hpack_huffman.h"
#undef HPH
	{0, 0, 33, 0} /* EOS */
};

static const struct hph *eos = &tbl[256];

static void dec_generate(const struct hph *, const struct hph *, uint32_t,
    int, int, const char *);

static int
dec_rndpow2(int i, int rnd)
{

	while ((i & (i - 1)) != 0)
		i += rnd;
	return (i);
}

static int
dec_bits(int i)
{
	int r = 0;

	i = dec_rndpow2(i, -1);
	while (i > 0) {
		i >>= 1;
		r++;
	}
	return (r);
}

static void
dec_pfxcat(char *dst, int ref, int n)
{

	n = dec_rndpow2(n, +1);
	assert(n >  0);
	assert(n <= 4);
	do {
		n--;
		strcat(dst, (ref >> n) & 1 ? "1" : "0");
	} while (n > 0);
}

static unsigned
dec_make_hit(const struct hph *hph, struct hph_dec *dec, unsigned n,
    uint32_t msk, int msk_len, int oct)
{
	uint32_t msb;

	msb = (uint32_t)(n << (32 - msk_len)) | msk;
	if ((msb >> (32 - hph->len)) != hph->cod)
		return (0);

	dec[n].len = (uint8_t)(hph->len - oct * 8);
	dec[n].chr = hph->chr;
	(void)snprintf(dec[n].nxt, sizeof dec->nxt, "NULL");
	return (1);
}

static unsigned
dec_make_misses(const struct hph *hph, struct hph_dec *dec, int n, int oct)
{
	const struct hph *max;
	char buf[sizeof "_pfxXXXX"];
	uint32_t msk, sub_msk;
	int sz, msk_len, sub_tbl, bits, ref;

	sz = dec_rndpow2(n, +1);
	sub_tbl = sz - n;
	assert(sub_tbl <= 10);

	bits = dec_bits(sub_tbl);
	sub_msk = (1 << bits) - 1;

	ref = (int)(1 + sub_msk - (unsigned)sub_tbl);
	while (n < sz) {
		assert(n < 256);
		dec[n].len = 8;
		dec[n].chr = 0;

		if (oct == 4) {
			dec[n].len = 0;
			(void)sprintf(dec[n].nxt, "NULL");
			break;
		}
		else {
			assert(oct < 4);
			(void)sprintf(buf, "_pfx");
			dec_pfxcat(buf, ref, bits - 1);
			(void)sprintf(dec[n].nxt, "&hph_dec%c%s", '0' + oct, buf);
		}

		if (n == 0xff) {
			/* start the next full octet */
			msk_len = 8 * (oct + 1);
			assert(msk_len > hph->len);
			msk = 0xffffffff << (40 - msk_len);
			max = eos;
			if (msk_len > 30)
				msk_len = 30;
		}
		else {
			/* help finish the next octect */
			msk_len = 8 * oct;
			msk = 0xffffffff << (32 - msk_len);
			msk |= (unsigned)ref << (32 - msk_len + bits);

			max = hph;
			while ((max->msb & msk) == (hph->msb & msk)) {
				msk_len = max->len;
				max++;
			}
			assert(max > hph);

			msk = hph->msb;
		}

		assert(max > hph);
		dec_generate(hph, max, msk, msk_len, oct, buf);
		hph = max;

		ref++;
		n++;
	}
	assert(dec_rndpow2(ref, +1) == ref);

	assert(sz > 0);
	return ((unsigned)sz);
}

static void
dec_generate(const struct hph *hph, const struct hph *max, uint32_t msk,
    int msk_len, int oct, const char *pfx)
{
	struct hph_dec dec[256];
	unsigned n, sz, len;

	n = 0;
	while (hph != max && hph->len <= msk_len) {
		assert(n < 256);
		if (!dec_make_hit(hph, dec, n, msk, msk_len, oct)) {
			assert(n > 0);
			hph++;
			continue;
		}
		n++;
	}
	assert(n > 0);

	if (max == eos)
		sz = dec_make_misses(hph, dec, (int)n, oct + 1);
	else {
		assert(hph == max);
		sz = n;
	}

	n = sz - 1;
	if (dec[n].len == 0)
		n--;
	len = dec[n].len;
	assert(len > 0);

	OUT("");
	GEN("static const struct hph_oct hph_oct%d%s[] = {", oct, pfx);
	for (n = 0; n < sz; n++)
		GEN("\t/* 0x%02x */ {%d, (char)0x%02x, %s},",
		    n, dec[n].len, dec[n].chr, dec[n].nxt);
	OUT("};");
	OUT("");
	GEN("static const struct hph_dec hph_dec%d%s = {%u, hph_oct%d%s};",
	    oct, pfx, len, oct, pfx);
}

int
main(void)
{

	GEN_HDR();
	OUT("struct hph_oct;");
	OUT("");
	OUT("struct hph_dec {");
	OUT("\tuint8_t\t\t\tlen;");
	OUT("\tconst struct hph_oct\t*oct;");
	OUT("};");
	OUT("");
	OUT("struct hph_oct {");
	OUT("\tuint8_t\t\t\tlen;");
	OUT("\tchar\t\t\tchr;");
	OUT("\tconst struct hph_dec\t*nxt;");
	OUT("};");

	dec_generate(tbl, eos, 0x00000000, 8, 0, "");

	return (0);
}
