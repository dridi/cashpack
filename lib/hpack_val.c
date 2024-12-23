/*-
 * License: BSD-2-Clause
 * (c) 2016-2024 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 *
 * Header field grammar validation (RFC 7230 Section 3.2)
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hpack.h"
#include "hpack_priv.h"

#define IS_VCHAR(c)		((uint8_t)c > 0x20 && (uint8_t)c < 0x7f)
#define IS_OBS_TEXT(c)		((uint8_t)c & 0x80)
#define IS_FIELD_VCHAR(c)	(IS_VCHAR(c) || IS_OBS_TEXT(c))
#define IS_FIELD_VALUE(c)	(c == ' ' || c == '\t' || IS_FIELD_VCHAR(c))

int
HPV_value(HPACK_CTX, const char *str, size_t len)
{
	uint8_t c;

	assert(str != NULL);
	while (len > 0) {
		c = (uint8_t)*str;
		/* RFC 7230 3.2.  Header Fields */
		EXPECT(ctx, CHR, c == '\t' || (c >= ' ' && c != 0x7f));
		assert(IS_FIELD_VALUE(c));
		str++;
		len--;
	}

	assert(*str == '\0');
	return (0);
}

int
HPV_token(HPACK_CTX, const char *str, size_t len)
{

	assert(str != NULL);
	assert(len > 0);

	if (ctx->hp->magic == DECODER_MAGIC && str == hpack_unknown_name) {
		assert(ctx->hp->flg & HPD_FLG_MON);
		return (0);
	}

	/* RFC 7540 Section 8.1.2.1.  Pseudo-Header Fields */
	if (*str == ':') {
#define HPPH(hdr)							\
		if (len == strlen(hdr) && !memcmp(str, hdr, len))	\
			return (0);
#include "tbl/hpack_pseudo_headers.h"
#undef HPPH
		ctx->res = HPACK_RES_HDR;
		return (HPACK_RES_HDR);
	}

	while (len > 0) {
		/* RFC 7230 Section 3.2.6.  Field Value Components */
		EXPECT(ctx, CHR, IS_VCHAR(*str) &&
		    strchr("()<>@,;:\\\"/[]?={} ", *str) == NULL);
		/* RFC 7540 Section 8.1.2.  HTTP Header Fields */
		EXPECT(ctx, CHR, *str < 'A' || *str > 'Z');
		str++;
		len--;
	}

	assert(*str == '\0');
	return (0);
}
