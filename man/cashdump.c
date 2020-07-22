/*-
 * Copyright (c) 2016-2017 Dridi Boukelmoune
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <hpack.h>

/* HTTP/2 declarations */

#define H2_TYP_HEADERS		0x01
#define H2_TYP_PUSH_PROMISE	0x05
#define H2_TYP_CONTINUATION	0x09

#define H2_FLG_END_HEADERS	0x04
#define H2_FLG_PADDED		0x08
#define H2_FLG_PRIORITY		0x20

struct h2frame {
	uint8_t	len[3];
	uint8_t	typ;
	uint8_t	flg;
	uint8_t	str[4];
};

/* utility functions */

static size_t
read_block(void *ptr, size_t len)
{
	size_t nmemb;

	nmemb = fread(ptr, len, 1, stdin);
	if (ferror(stdin)) {
		perror("fread");
		exit(EXIT_FAILURE);
	}

	return (nmemb);
}

static void
skip_block(void *ptr, size_t len)
{

	if (read_block(ptr, len) == 0) {
		perror("fread");
		exit(EXIT_FAILURE);
	}
}

static void
print_error(const char *func, enum hpack_result_e res)
{

	fprintf(stderr, "%s: %s\n", func, hpack_strerror(res));
	exit(EXIT_FAILURE);
}

/* callback for cashpack */

static void
print_headers(enum hpack_event_e evt, const char *buf, size_t len, void *priv)
{

	(void)priv;

	/* print "\n${name}: ${value}" for each header field */

	switch (evt) {
	case HPACK_EVT_FIELD:
		printf("\n");
		break;
	case HPACK_EVT_VALUE:
		printf(": ");
		/* fall through */
	case HPACK_EVT_NAME:
		printf("%s", buf);
		(void)len;
		/* fall through */
	default:
		/* ignore other events */
		break;
	}
}

/* cashdump */

int
main(int argc, char **argv)
{
	enum hpack_result_e res;
	struct h2frame frm;
	struct hpack *hp;
	struct hpack_decoding dec;
	uint8_t blk[4096], buf[1024], flg, pad;
	uint32_t str;
	unsigned first;
	size_t len;

	/* command-line arguments are not used */
	(void)argc;
	(void)argv;

	/* initialization */
	first = 1;
	hp = hpack_decoder(4096, -1, hpack_default_alloc, 0);
	dec.blk = blk;
	dec.blk_len = 0;
	dec.buf = buf;
	dec.buf_len = sizeof buf;
	dec.cb = print_headers;
	dec.priv = NULL;

	while (read_block(&frm, sizeof frm) == 1) {
		if (!first)
			puts("\n");
		first = 0;

		/* decode the HTTP/2 frame */
		pad = 0;
		len = frm.len[0] << 16 | frm.len[1] << 8 | frm.len[2];
		str = frm.str[0] << 24 | frm.str[1] << 16 | frm.str[2] << 8 |
		    frm.str[3];

		if (len > sizeof blk)
			return (EXIT_FAILURE); /* DIY */

		flg = frm.flg;
		if (flg & H2_FLG_PADDED)
			skip_block(&pad, 1);
		if (flg & H2_FLG_PRIORITY)
			skip_block(blk, 5);

		skip_block(blk, len);
		if (frm.typ == H2_TYP_PUSH_PROMISE ||
		    frm.typ == H2_TYP_CONTINUATION)
			return (EXIT_FAILURE); /* DIY */

		if (frm.typ != H2_TYP_HEADERS)
			continue;

		/* decode the HPACK block */
		if (flg & H2_FLG_END_HEADERS)
			printf("=== stream %u", str);

		dec.cut = ~flg & H2_FLG_END_HEADERS;
		dec.blk_len = len;
		res = hpack_decode(hp, &dec);
		if (res < 0)
			print_error("hpack_decode", res);

		if (flg & H2_FLG_PADDED)
			skip_block(blk, pad);
	}

	if (!feof(stdin)) {
		fprintf(stderr, "error: incomplete frame\n");
		return (EXIT_FAILURE);
	}

	printf("\n\n=== dynamic table");
	res = hpack_dynamic(hp, print_headers, NULL);
	if (res < 0)
		print_error("hpack_dynamic", res);

	hpack_free(&hp);

	return (EXIT_SUCCESS);
}
