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
 *
 * Make all kinds of arguments passing outside of the happy and default paths.
 */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hpack.h"
#include "dbg.h"

/**********************************************************************
 * Utility macros
 */

#define CHECK_NULL(res, call, args...)	\
	do {				\
		res = call(args);	\
		assert(res == NULL);	\
		(void)res;		\
	} while (0)

#define CHECK_NOTNULL(res, call, args...)	\
	do {					\
		res = call(args);		\
		assert(res != NULL);		\
		(void)res;			\
	} while (0)

#define CHECK_RES(res, exp, call, args...)			\
	do {							\
		res = call(args);				\
		DBG("expected %s, got %d (%s)", #exp, res,	\
		    hpack_strerror(res));			\
		assert(res == HPACK_RES_##exp);			\
		(void)res;					\
	} while (0)

/**********************************************************************
 * Data structures
 */

static const struct hpack_alloc null_alloc = { NULL, NULL, NULL, NULL };

static const uint8_t basic_block[] = { 0x82 };

static const uint8_t junk_block[] = { 0x80 };

static const uint8_t update_block[] = { 0x20 };

static const uint8_t double_block[] = { 0x82, 0x84 };

static const struct hpack_field basic_field[] = {{
	.flg = HPACK_FLG_TYP_IDX,
	.idx = 1,
}};

static const struct hpack_field unknown_field[] = {{
	.flg = 0xff,
	.idx = 1,
}};

/**********************************************************************
 * Utility functions
 */

static void
noop_cb(enum hpack_event_e evt, const char *buf, size_t len, void *priv)
{

	assert(priv == NULL);
	(void)priv;
	(void)evt;
	(void)buf;
	(void)len;
}

static struct hpack *
make_decoder(size_t max, ssize_t rsz, const struct hpack_alloc *ha)
{
	struct hpack *hp;

	CHECK_NOTNULL(hp, hpack_decoder, max, rsz, ha);
	return (hp);
}

static struct hpack *
make_encoder(size_t max, ssize_t lim, const struct hpack_alloc *ha)
{
	struct hpack *hp;

	CHECK_NOTNULL(hp, hpack_encoder, max, lim, ha);
	return (hp);
}

/**********************************************************************
 * Static allocator
 */

static uint8_t static_buffer[1024];

static void *
static_malloc(size_t size, void *priv)
{

	(void)priv;
	if (size > sizeof static_buffer)
		return (NULL);

	return (&static_buffer);
}

static const struct hpack_alloc static_alloc = {
	static_malloc,
	NULL,
	NULL,
	NULL
};

/**********************************************************************
 * Out of memory allocator
 */

static void *
oom_malloc(size_t size, void *priv)
{

	(void)priv;
	return (malloc(size));
}

static void *
oom_realloc(void *ptr, size_t size, void *priv)
{

	(void)ptr;
	(void)size;
	(void)priv;
	return (NULL);
}

static void
oom_free(void *ptr, void *priv)
{

	(void)priv;
	free(ptr);
}


static const struct hpack_alloc oom_alloc = {
	oom_malloc,
	oom_realloc,
	oom_free,
	NULL
};

/**********************************************************************
 * Test cases sharing a bunch of global variables
 */

struct hpack *hp;
struct hpack_field fld;
char wrk_buf[256];
int retval;

#define DECODING(nam)				\
const struct hpack_decoding nam##_decoding = {	\
	.blk = nam##_block,			\
	.blk_len = sizeof nam##_block,		\
	.buf = wrk_buf,				\
	.buf_len = sizeof wrk_buf,		\
	.cb = noop_cb,				\
	.priv = NULL,				\
}
DECODING(update);
DECODING(junk);
DECODING(double);
#undef DECODING

const struct hpack_encoding basic_encoding = {
	.fld = basic_field,
	.fld_cnt = 1,
	.buf = wrk_buf,
	.buf_len = sizeof wrk_buf,
	.cb = noop_cb,
	.priv = NULL,
};

const struct hpack_encoding unknown_encoding = {
	.fld = unknown_field,
	.fld_cnt = 1,
	.buf = wrk_buf,
	.buf_len = sizeof wrk_buf,
	.cb = noop_cb,
	.priv = NULL,
};

static void
test_null_alloc(void)
{
	CHECK_NULL(hp, hpack_decoder, 0, -1, NULL);
}

static void
test_null_malloc(void)
{
	CHECK_NULL(hp, hpack_decoder, 0, -1, &null_alloc);
}

static void
test_null_free(void)
{
	hp = make_decoder(0, -1, &static_alloc);
	hpack_free(&hp);
}

static void
test_alloc_overflow(void)
{
	CHECK_NULL(hp, hpack_decoder, UINT16_MAX + 1, -1,
	    hpack_default_alloc);
	CHECK_NULL(hp, hpack_decoder, 4096, UINT16_MAX + 1,
	    hpack_default_alloc);
}

static void
test_alloc_underflow(void)
{
	CHECK_NOTNULL(hp, hpack_decoder, 4096, 2048, hpack_default_alloc);
	hpack_free(&hp);
	CHECK_NOTNULL(hp, hpack_encoder, 4096, 2048, hpack_default_alloc);
	hpack_free(&hp);
}

static void
test_malloc_failure(void)
{
	CHECK_NULL(hp, hpack_decoder, 4096, -1, &static_alloc);
	CHECK_NULL(hp, hpack_encoder, 0,  4096, &static_alloc);
}

static void
test_free_null_codec(void)
{
	assert(hp == NULL);
	hpack_free(&hp);
	hpack_free(NULL);
}

static void
test_double_free(void)
{
	struct hpack *hp2;

	hp = make_decoder(0, -1, &static_alloc);
	hp2 = hp;
	hpack_free(&hp);
	hpack_free(&hp2);
}

static void
test_foreach(void)
{
	hp = make_decoder(0, -1, &static_alloc);
	CHECK_RES(retval, OK, hpack_static, noop_cb, NULL);
	CHECK_RES(retval, OK, hpack_dynamic, hp, noop_cb, NULL);
	CHECK_RES(retval, OK, hpack_tables, hp, noop_cb, NULL);
	hpack_free(&hp);
}

static void
test_foreach_null_args(void)
{
	hp = make_decoder(0, -1, hpack_default_alloc);
	CHECK_RES(retval, ARG, hpack_dynamic, NULL, NULL, NULL);
	CHECK_RES(retval, ARG, hpack_dynamic, hp, NULL, NULL);
	CHECK_RES(retval, ARG, hpack_static, NULL, NULL);
	CHECK_RES(retval, ARG, hpack_tables, NULL, NULL, NULL);
	hpack_free(&hp);
}

static void
test_decode_null_args(void)
{
	struct hpack_decoding dec;

	/* first attempt when everything is null */
	(void)memset(&dec, 0, sizeof dec);
	CHECK_RES(retval, ARG, hpack_decode, hp, &dec, 0);

	hp = make_decoder(512, -1, hpack_default_alloc);

	CHECK_RES(retval, ARG, hpack_decode, hp, NULL, 0);

	/* populate dec members one by one */
	CHECK_RES(retval, ARG, hpack_decode, hp, &dec, 0);
	dec.blk = basic_block;
	CHECK_RES(retval, ARG, hpack_decode, hp, &dec, 0);
	dec.blk_len = sizeof basic_block;
	CHECK_RES(retval, ARG, hpack_decode, hp, &dec, 0);
	dec.buf = wrk_buf;
	CHECK_RES(retval, ARG, hpack_decode, hp, &dec, 0);
	dec.buf_len = sizeof wrk_buf;
	CHECK_RES(retval, ARG, hpack_decode, hp, &dec, 0);

	hpack_free(&hp);
}

static void
test_encode_null_args(void)
{
	struct hpack_encoding enc;

	/* first attempt when everything is null */
	(void)memset(&enc, 0, sizeof enc);
	CHECK_RES(retval, ARG, hpack_encode, hp, &enc, 0);

	hp = make_encoder(512, -1, hpack_default_alloc);

	CHECK_RES(retval, ARG, hpack_encode, hp, NULL, 0);

	/* populate enc members one by one */
	(void)memset(&enc, 0, sizeof enc);
	CHECK_RES(retval, ARG, hpack_encode, hp, &enc, 0);
	enc.fld = basic_field;
	CHECK_RES(retval, ARG, hpack_encode, hp, &enc, 0);
	enc.fld_cnt = 1;
	CHECK_RES(retval, ARG, hpack_encode, hp, &enc, 0);
	enc.buf = wrk_buf;
	CHECK_RES(retval, ARG, hpack_encode, hp, &enc, 0);
	enc.buf_len = sizeof wrk_buf;
	CHECK_RES(retval, ARG, hpack_encode, hp, &enc, 0);

	hpack_free(&hp);
}

static void
test_resize_overflow(void)
{
	hp = make_decoder(0, -1, &static_alloc);
	CHECK_RES(retval, LEN, hpack_resize, &hp, UINT16_MAX + 1);
	hpack_free(&hp);
}

static void
test_limit_null_realloc(void)
{
	hp = make_encoder(4096, 0, &static_alloc);

	CHECK_RES(retval, OK, hpack_encode, hp, &basic_encoding, 0);
	CHECK_RES(retval, ARG, hpack_limit, &hp, 4096);
	hpack_free(&hp);
}

static void
test_limit_realloc_failure(void)
{
	hp = make_encoder(4096, 2048, &oom_alloc);
	CHECK_RES(retval, OK, hpack_encode, hp, &basic_encoding, 0);
	CHECK_RES(retval, OOM, hpack_limit, &hp, 4096);
	hpack_free(&hp);
}

static void
test_trim_null_realloc(void)
{
	hp = make_decoder(0, -1, &static_alloc);
	CHECK_RES(retval, ARG, hpack_trim, &hp);
	hpack_free(&hp);
}

static void
test_trim_realloc_failure(void)
{
	hp = make_decoder(4096, -1, &oom_alloc);
	CHECK_RES(retval, OK, hpack_resize, &hp, 0);
	CHECK_RES(retval, OK, hpack_decode, hp, &update_decoding, 0);
	CHECK_RES(retval, OOM, hpack_trim, &hp);
	hpack_free(&hp);
}

static void
test_use_defunct_decoder(void)
{
	hp = make_decoder(0, -1, hpack_default_alloc);

	/* break the decoder */
	CHECK_RES(retval, IDX, hpack_decode, hp, &junk_decoding, 0);

	/* try using it again */
	CHECK_RES(retval, ARG, hpack_decode, hp, &junk_decoding, 0);

	hpack_free(&hp);
}

static void
test_use_busy_decoder(void)
{
	hp = make_decoder(0, -1, hpack_default_alloc);
	CHECK_RES(retval, BLK, hpack_decode, hp, &double_decoding, 1);
	CHECK_RES(retval, BSY, hpack_resize, &hp, 0);
	CHECK_RES(retval, BSY, hpack_trim, &hp);
	CHECK_RES(retval, BSY, hpack_dynamic, hp, noop_cb, NULL);
	hpack_free(&hp);
}

static void
test_use_different_buffer(void)
{
	struct hpack_decoding different_decoding;

	hp = make_decoder(0, -1, hpack_default_alloc);
	CHECK_RES(retval, BLK, hpack_decode, hp, &double_decoding, 1);
	different_decoding = double_decoding;
	different_decoding.buf_len -= 64;
	CHECK_RES(retval, ARG, hpack_decode, hp, &different_decoding, 1);
	hpack_free(&hp);
}

static void
test_limit_null_encoder(void)
{
	assert(hp == NULL);
	CHECK_RES(retval, ARG, hpack_limit, &hp, 0);
	CHECK_RES(retval, ARG, hpack_limit, NULL, 0);
}

static void
test_limit_decoder(void)
{
	hp = make_decoder(4096, -1, hpack_default_alloc);
	CHECK_RES(retval, ARG, hpack_limit, &hp, 0);
	hpack_free(&hp);
}

static void
test_limit_overflow(void)
{
	hp = make_encoder(0, -1, hpack_default_alloc);
	CHECK_RES(retval, LEN, hpack_limit, &hp, UINT16_MAX + 1);
	hpack_free(&hp);
}

static void
test_limit_overflow_no_realloc(void)
{
	hp = make_encoder(0, -1, &static_alloc);
	CHECK_RES(retval, LEN, hpack_limit, &hp, UINT16_MAX + 1);
	hpack_free(&hp);
}

static void
test_limit_between_two_resizes(void)
{
	hp = make_encoder(512, -1, &static_alloc);
	CHECK_RES(retval, OK, hpack_limit, &hp, 256);
	CHECK_RES(retval, OK, hpack_resize, &hp, 1024);
	CHECK_RES(retval, OK, hpack_encode, hp, &basic_encoding, 0);
	CHECK_RES(retval, OK, hpack_resize, &hp, 2048);
	hpack_free(&hp);
}

static void
test_use_defunct_encoder(void)
{
	hp = make_encoder(4096, -1, hpack_default_alloc);

	/* break the decoder */
	CHECK_RES(retval, ARG, hpack_encode, hp, &unknown_encoding, 0);

	/* try using it again */
	CHECK_RES(retval, ARG, hpack_encode, hp, &unknown_encoding, 0);

	/* try resizing it */
	CHECK_RES(retval, ARG, hpack_resize, &hp, 0);

	/* try trimming it */
	CHECK_RES(retval, ARG, hpack_trim, &hp);
	hpack_free(&hp);
}

static void
test_use_busy_encoder(void)
{
	hp = make_encoder(0, -1, hpack_default_alloc);
	CHECK_RES(retval, BLK, hpack_encode, hp, &basic_encoding, 1);
	CHECK_RES(retval, BSY, hpack_resize, &hp, 0);
	CHECK_RES(retval, BSY, hpack_limit, &hp, 0);
	CHECK_RES(retval, BSY, hpack_trim, &hp);
	CHECK_RES(retval, BSY, hpack_dynamic, hp, noop_cb, NULL);
	hpack_free(&hp);
}

static void
test_resize_null_codec(void)
{
	assert(hp == NULL);
	CHECK_RES(retval, ARG, hpack_resize, NULL, 0);
	CHECK_RES(retval, ARG, hpack_resize, &hp, 0);
}

static void
test_trim_null_codec(void)
{
	assert(hp == NULL);
	CHECK_RES(retval, ARG, hpack_trim, NULL);
	CHECK_RES(retval, ARG, hpack_trim, &hp);
}

static void
test_trim_to_limit(void)
{
	hp = make_encoder(512, -1, hpack_default_alloc);
	CHECK_RES(retval, OK, hpack_limit, &hp, 256);
	CHECK_RES(retval, OK, hpack_encode, hp, &basic_encoding, 0);
	CHECK_RES(retval, OK, hpack_trim, &hp);
	hpack_free(&hp);
}

static void
test_resize_realloc_failure(void)
{
	hp = make_decoder(0, -1, &oom_alloc);
	CHECK_RES(retval, OOM, hpack_resize, &hp, UINT16_MAX);
	hpack_free(&hp);
}

static void
test_clean_null_field(void)
{
	CHECK_RES(retval, ARG, hpack_clean_field, NULL);
}

static void
test_clean_unknown_field(void)
{
	fld = *unknown_field;
	hpack_clean_field(&fld);
}

static void
test_clean_indexed_field_with_name(void)
{
	(void)memset(&fld, 0, sizeof fld);
	fld.flg = HPACK_FLG_TYP_IDX;
	fld.nam = "";
	CHECK_RES(retval, ARG, hpack_clean_field, &fld);
}

static void
test_clean_indexed_field_with_value(void)
{
	(void)memset(&fld, 0, sizeof fld);
	fld.flg = HPACK_FLG_TYP_IDX;
	fld.val = "";
	CHECK_RES(retval, ARG, hpack_clean_field, &fld);
}

static void
test_clean_indexed_field_with_indexed_name(void)
{
	(void)memset(&fld, 0, sizeof fld);
	fld.flg = HPACK_FLG_TYP_IDX | HPACK_FLG_NAM_IDX;
	CHECK_RES(retval, ARG, hpack_clean_field, &fld);
}

static void
test_strerror(void)
{
	const char *str;

	CHECK_NOTNULL(str, hpack_strerror, HPACK_RES_OK);
	CHECK_NOTNULL(str, hpack_strerror, HPACK_RES_OOM);
	CHECK_NULL(str, hpack_strerror, UINT16_MAX);
}

/**********************************************************************
 */

int
main(int argc, char **argv)
{

	(void)argc;
	(void)argv;

	test_null_alloc();
	test_null_malloc();
	test_null_free();
	test_alloc_overflow();
	test_alloc_underflow();
	test_malloc_failure();
	test_free_null_codec();
	test_double_free();

	test_foreach();
	test_foreach_null_args();
	test_decode_null_args();
	test_encode_null_args();

	test_resize_overflow();
	test_limit_null_realloc();
	test_limit_realloc_failure();
	test_trim_null_realloc();
	test_trim_realloc_failure();

	test_use_defunct_decoder();
	test_use_busy_decoder();
	test_use_different_buffer();

	test_limit_null_encoder();
	test_limit_decoder();
	test_limit_overflow();
	test_limit_overflow_no_realloc();
	test_limit_between_two_resizes();

	test_use_defunct_encoder();
	test_use_busy_encoder();

	test_resize_null_codec();
	test_trim_null_codec();
	test_trim_to_limit();

	test_resize_realloc_failure();

	test_clean_null_field();
	test_clean_unknown_field();

	test_clean_indexed_field_with_name();
	test_clean_indexed_field_with_value();
	test_clean_indexed_field_with_indexed_name();

	test_strerror();

	return (0);
}
