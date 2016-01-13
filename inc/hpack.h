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
 * HPACK: Header Compression for HTTP/2 (RFC 7541)
 */

struct hpack;

enum hpack_res_e {
	HPACK_RES_DEV	=  1,
	HPACK_RES_OK	=  0,
	HPACK_RES_ARG	= -1,
	HPACK_RES_BUF	= -2,
	HPACK_RES_INT	= -3,
	HPACK_RES_IDX	= -4,
	HPACK_RES_LEN	= -5,
};

enum hpack_evt_e {
	HPACK_EVT_FIELD	= 0,
	HPACK_EVT_NEVER	= 1,
	HPACK_EVT_INDEX	= 2,
	HPACK_EVT_NAME	= 3,
	HPACK_EVT_VALUE	= 4,
	HPACK_EVT_DATA	= 5,
	HPACK_EVT_TABLE	= 6,
};

typedef void hpack_decoded_f(void *, enum hpack_evt_e, const void *, size_t);

struct hpack * HPACK_encoder(size_t);
struct hpack * HPACK_decoder(size_t);
void HPACK_free(struct hpack **);

enum hpack_res_e HPACK_decode(struct hpack *, const void *, size_t,
    hpack_decoded_f cb, void *priv);
