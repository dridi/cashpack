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

static const uint8_t basic_frame[] = { 0x82 };

static const uint8_t junk_frame[] = { 0x80 };

/**********************************************************************
 * Utility functions
 */

static void
noop_cb(void *priv, enum hpack_evt_e evt, const char *buf, size_t len)
{

	assert(priv == NULL);
	(void)priv;
	(void)evt;
	(void)buf;
	(void)len;
}

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
 */

int
main(int argc, char **argv)
{
	struct hpack *hp;
	int retval;

	(void)argc;
	(void)argv;

	/* codec allocation */
	CHECK_NULL(hp, hpack_decoder, 0, NULL);
	CHECK_NULL(hp, hpack_decoder, 0, &null_alloc);

	CHECK_NULL(hp, hpack_decoder, 4096, &static_alloc);

	CHECK_NOTNULL(hp, hpack_decoder, 0, &static_alloc);
	hpack_free(&hp);

	assert(hp == NULL);
	hpack_free(&hp);

	/* decoding process */
	CHECK_NOTNULL(hp, hpack_decoder, 0, hpack_default_alloc);
	CHECK_RES(retval, ARG, hpack_decode, NULL, NULL, 0, NULL, NULL);
	CHECK_RES(retval, ARG, hpack_decode, hp, NULL, 0, NULL, NULL);
	CHECK_RES(retval, ARG, hpack_decode, hp, basic_frame, 0, NULL, NULL);
	CHECK_RES(retval, ARG, hpack_decode, hp, basic_frame,
	    sizeof basic_frame, NULL, NULL);

	/* defunct decoder */
	CHECK_RES(retval, IDX, hpack_decode, hp, junk_frame,
	    sizeof junk_frame, noop_cb, NULL);
	CHECK_RES(retval, ARG, hpack_decode, hp, junk_frame,
	    sizeof junk_frame, noop_cb, NULL);

	hpack_free(&hp);

	return (0);
}
