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

/**********************************************************************
 * Data Structures
 */

enum hpack_type_e {
	HPACK_INDEXED	= 0x80, /* Section 6.1 */
	HPACK_DYNAMIC	= 0x40, /* Section 6.2.1 */
	HPACK_LITERAL	= 0x00, /* Section 6.2.2 */
	HPACK_NEVER	= 0x10, /* Section 6.2.3 */
	HPACK_UPDATE	= 0x20, /* Section 6.3 */
};

enum hpack_encoding_e { /* Section 5.2 */
	HPACK_HUFFMAN	= 0x80,
	HPACK_RAW	= 0x00,
};

struct hpt_field {
	char		*nam;
	uint8_t		*val;
	uint16_t	nam_sz;
	uint16_t	val_sz;
};

struct hpt_entry {
	uint16_t	pre_sz;
	uint16_t	nam_sz;
	uint16_t	val_sz;
	uint16_t	pad[13];
};

struct hpt_priv {
	struct hpack_ctx	*ctx;
	size_t			len;
};

struct hpack {
	uint32_t		magic;
#define ENCODER_MAGIC		0x8ab1fb4c
#define DECODER_MAGIC		0xab0e3218
	size_t			max;
	size_t			lim;
	size_t			len;
	size_t			cnt;
	struct hpt_entry	tbl[0];
};

struct hpack_ctx {
	enum hpack_res_e	res;
	struct hpack		*hp;
	const uint8_t		*buf;
	size_t			len;
	hpack_decoded_f		*cb;
	void			*priv;
};

/**********************************************************************
 * Utility Macros
 */

#define HPACK_CTX	struct hpack_ctx *ctx

#define CALL(func, ...)				\
	do {					\
		if ((func)(__VA_ARGS__) != 0)	\
			return (-1);		\
	} while (0)

#define CALLBACK(ctx, ...)				\
	do {						\
		(ctx)->cb((ctx)->priv, __VA_ARGS__);	\
	} while (0)

#define EXPECT(ctx, err, cond)				\
	do {						\
		if (!(cond)) {				\
			(ctx)->res = HPACK_RES_##err;	\
			return (-1);			\
		}					\
	} while (0)

#define INCOMPL(ctx)	EXPECT(ctx, DEV, 0)

#define WRONG(str)		\
	do {			\
		assert(!str);	\
	} while (0)

/**********************************************************************
 * Function Signatures
 */

int HPI_decode(HPACK_CTX, size_t, uint16_t *);

hpack_decoded_f HPT_insert;
int HPT_decode(HPACK_CTX, size_t);
int HPT_decode_name(HPACK_CTX, size_t);
