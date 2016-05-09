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

#define CHECK_RES(res, exp, call, args...)	\
	do {					\
		res = call(args);		\
		assert(res == HPACK_RES_##exp);	\
		(void)res;			\
	} while (0)

/**********************************************************************
 * Data structures
 */

static const struct hpack_alloc null_alloc = { NULL, NULL, NULL };

static const uint8_t basic_block[] = { 0x82 };

static const uint8_t junk_block[] = { 0x80 };

static const uint8_t update_block[] = { 0x20 };

static const struct hpack_item basic_item = {
	.typ = HPACK_INDEXED,
	.idx = 1,
};

static const struct hpack_item unknown_item = {
	.typ = 0xff,
	.idx = 1,
};

/**********************************************************************
 * Utility functions
 */

static void
noop_cb(void *priv, enum hpack_evt_e evt, const void *buf, size_t len)
{

	assert(priv == NULL);
	(void)priv;
	(void)evt;
	(void)buf;
	(void)len;
}

static hpack_decoded_f *noop_dec_cb = (hpack_decoded_f *)noop_cb;
static hpack_encoded_f *noop_enc_cb = (hpack_encoded_f *)noop_cb;

/**********************************************************************
 * Static allocator
 */

static uint8_t static_buffer[128];

static void *
static_malloc(size_t size)
{
	if (size > sizeof static_buffer)
		return (NULL);

	return (&static_buffer);
}

static const struct hpack_alloc static_alloc = { static_malloc, NULL, NULL };

/**********************************************************************
 * Out of memory allocator
 */

static void *
oom_realloc(void *ptr, size_t size)
{

	(void)ptr;
	(void)size;
	return (NULL);
}

static const struct hpack_alloc oom_alloc = { malloc, oom_realloc, free };

/**********************************************************************
 */

int
main(int argc, char **argv)
{
	struct hpack *hp;
	struct hpack_item itm;
	int retval;

	(void)argc;
	(void)argv;

	/* codec allocation */
	CHECK_NULL(hp, hpack_decoder, 0, NULL);
	CHECK_NULL(hp, hpack_decoder, 0, &null_alloc);

	CHECK_NULL(hp, hpack_decoder, UINT16_MAX + 1, hpack_default_alloc);

	CHECK_NULL(hp, hpack_decoder, 4096, &static_alloc);

	CHECK_NOTNULL(hp, hpack_decoder, 0, &static_alloc);
	hpack_free(&hp);

	assert(hp == NULL);
	hpack_free(&hp);

	hpack_free(NULL);

	/* dynamic table inspection */
	CHECK_NOTNULL(hp, hpack_decoder, 0, hpack_default_alloc);
	CHECK_RES(retval, ARG, hpack_foreach, NULL, NULL, NULL);
	CHECK_RES(retval, ARG, hpack_foreach, hp, NULL, NULL);
	hpack_free(&hp);

	/* decoding process */
	CHECK_NOTNULL(hp, hpack_decoder, 0, &static_alloc);
	CHECK_RES(retval, ARG, hpack_decode, NULL, NULL, 0, 0, NULL, NULL);
	CHECK_RES(retval, ARG, hpack_decode, hp, NULL, 0, 0, NULL, NULL);
	CHECK_RES(retval, ARG, hpack_decode, hp, basic_block, 0, 0, NULL,
	    NULL);
	CHECK_RES(retval, ARG, hpack_decode, hp, basic_block,
	    sizeof basic_block, 0, NULL, NULL);

	/* over-resize/trim decoder with no realloc */
	CHECK_RES(retval, LEN, hpack_resize, &hp, UINT16_MAX + 1);
	CHECK_RES(retval, ARG, hpack_trim, &hp);
	hpack_free(&hp);

	/* fail to trim decoder */
	CHECK_NOTNULL(hp, hpack_decoder, 4096, &oom_alloc);
	CHECK_RES(retval, OK, hpack_resize, &hp, 0);
	CHECK_RES(retval, OK, hpack_decode, hp, update_block,
	    sizeof update_block, 0, noop_dec_cb, NULL);
	CHECK_RES(retval, OOM, hpack_trim, &hp);
	hpack_free(&hp);

	/* defunct decoder */
	CHECK_NOTNULL(hp, hpack_decoder, 0, hpack_default_alloc);
	CHECK_RES(retval, IDX, hpack_decode, hp, junk_block,
	    sizeof junk_block, 0, noop_dec_cb, NULL);
	CHECK_RES(retval, ARG, hpack_decode, hp, junk_block,
	    sizeof junk_block, 0, noop_dec_cb, NULL);

	hpack_free(&hp);

	/* encoding process */
	CHECK_NOTNULL(hp, hpack_encoder, 0, hpack_default_alloc);
	CHECK_RES(retval, ARG, hpack_encode, NULL, NULL, 0, NULL, NULL);
	CHECK_RES(retval, ARG, hpack_encode, hp, NULL, 0, NULL, NULL);
	CHECK_RES(retval, ARG, hpack_encode, hp, &basic_item, 0, NULL, NULL);
	CHECK_RES(retval, ARG, hpack_encode, hp, &basic_item, 1, NULL, NULL);

	/* defunct encoder */
	CHECK_RES(retval, ARG, hpack_encode, hp, &unknown_item, 1,
	    noop_enc_cb, NULL);
	CHECK_RES(retval, ARG, hpack_encode, hp, &unknown_item, 1,
	    noop_enc_cb, NULL);

	/* resize/trim defunct encoder */
	CHECK_RES(retval, ARG, hpack_encode, hp, &unknown_item, 1,
	    noop_enc_cb, NULL);
	CHECK_RES(retval, ARG, hpack_resize, &hp, 0);
	CHECK_RES(retval, ARG, hpack_trim, &hp);
	hpack_free(&hp);

	/* resize/trim null encoder */
	CHECK_RES(retval, ARG, hpack_resize, NULL, 0);
	CHECK_RES(retval, ARG, hpack_resize, &hp, 0);

	CHECK_RES(retval, ARG, hpack_trim, NULL);
	CHECK_RES(retval, ARG, hpack_trim, &hp);

	/* fail to resize when out of memory */
	CHECK_NOTNULL(hp, hpack_decoder, 0, &oom_alloc);
	retval = hpack_resize(&hp, UINT16_MAX);
	assert(retval == HPACK_RES_OOM);
	hpack_free(&hp);

	/* clean broken items */
	CHECK_RES(retval, ARG, hpack_clean_item, NULL);

	itm = unknown_item;
	hpack_clean_item(&itm);

	(void)memset(&itm, 0, sizeof itm);
	itm.typ = HPACK_INDEXED;
	itm.fld.nam = "";
	CHECK_RES(retval, ARG, hpack_clean_item, &itm);

	(void)memset(&itm, 0, sizeof itm);
	itm.typ = HPACK_INDEXED;
	itm.fld.val = "";
	CHECK_RES(retval, ARG, hpack_clean_item, &itm);

	(void)memset(&itm, 0, sizeof itm);
	itm.typ = HPACK_INDEXED;
	itm.fld.flg = HPACK_IDX;
	CHECK_RES(retval, ARG, hpack_clean_item, &itm);

	/* strerror */
	(void)hpack_strerror(HPACK_RES_OK);
	(void)hpack_strerror(HPACK_RES_OOM);
	(void)hpack_strerror(UINT16_MAX);

	return (0);
}
