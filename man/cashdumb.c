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

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <hpack.h>

/* utility functions */

#define LOG(...) fprintf(stderr, __VA_ARGS__)

static void
dumb_perror(const char *func, int retval)
{

	LOG("%s: %s\n", func, hpack_strerror(retval));
	exit(EXIT_FAILURE);
}

/* data structures */

struct dumb_field {
	char	*nam;
	char	*val;
};

struct dumb_state {
	struct dumb_field	*dmb;
	struct hpack_field	*fld;
	size_t			len;
	size_t			pos;
	size_t			idx_len;
	size_t			idx_off;
};

/* global variables for convenience */

#define TABLE_SIZE 256
#define MAX_FIELDS 7

static struct hpack_field static_fld[MAX_FIELDS];
static struct dumb_field  static_dmb[MAX_FIELDS];

/* logging of the encoding process */

static void
dumb_log_cb(enum hpack_event_e evt, const char *buf, size_t len, void *priv)
{
	struct hpack_field *hf;
	struct dumb_state *stt;

	stt = priv;
	hf = stt->fld + stt->pos;

	switch (evt) {
	case HPACK_EVT_FIELD:
		stt->pos++;
		LOG("encoding field: %s: %s ", hf->nam, hf->val);
		if (hf->flg & HPACK_FLG_TYP_IDX) {
			LOG("(indexed: %hu", hf->idx);
			if (hf->idx > stt->idx_len) {
				LOG(", expired");
				hf->flg = HPACK_FLG_TYP_DYN;
				hf->idx = 0;
			}
			else {
				hf->nam = NULL;
				hf->val = NULL;
			}
			if (hf->idx > HPACK_STATIC)
				hf->idx += stt->idx_off;
			LOG(")");
		}
		if (hf->flg & HPACK_FLG_NAM_IDX) {
			LOG("(indexed: %hu", hf->nam_idx);
			if (hf->nam_idx > stt->idx_len) {
				LOG(", expired");
				hf->flg = HPACK_FLG_TYP_DYN;
				hf->nam_idx = 0;
			}
			else
				hf->nam = NULL;
			if (hf->nam_idx > HPACK_STATIC)
				hf->nam_idx += stt->idx_off;
			LOG(")");
		}
		LOG("\n");
		break;
	case HPACK_EVT_EVICT:
		LOG("eviction\n");
		stt->idx_len--;
		break;
	case HPACK_EVT_INDEX:
		LOG("field indexed\n");
		stt->idx_off++;
		break;
	case HPACK_EVT_DATA:
		fwrite(buf, len, 1, stdout);
		/* fall through */
	default:
		/* ignore other events */
		break;
	}
}

/* fields management */

static void
dumb_fields_clear(struct dumb_state *stt)
{
	struct hpack_field *fld;
	struct dumb_field *dmb;
	int retval;

	dmb = stt->dmb;
	fld = stt->fld;

	while (stt->len > 0) {
		free(dmb->nam);
		free(dmb->val);
		retval = hpack_clean_field(fld);
		if (retval < 0)
			dumb_perror("hpack_clean_field", retval);
		dmb++;
		fld++;
		stt->len--;
	}
}

static void
dumb_fields_send(struct hpack *hp, struct dumb_state *stt, unsigned cut)
{
	struct hpack_encoding enc;
	char buf[256];
	int retval;

	stt->pos = 0;
	stt->idx_off = 0;

	enc.fld = stt->fld;
	enc.fld_cnt = stt->len;
	enc.buf = buf;
	enc.buf_len = sizeof buf;
	enc.cb = dumb_log_cb;
	enc.priv = stt;
	enc.cut = cut;

	retval = hpack_encode(hp, &enc);
	if (retval < 0)
		dumb_perror("hpack_encode", retval);

	dumb_fields_clear(stt);
	LOG("\n");
}

static void
dumb_fields_append(struct hpack *hp, struct dumb_state *stt, const char *line)
{
	struct hpack_field *fld;
	struct dumb_field *dmb;
	enum hpack_result_e res;
	const char *sep, *val;
	char *nam, *tmp;
	size_t idx;

	dmb = stt->dmb + stt->len;
	fld = stt->fld + stt->len;
	stt->len++;

	(void)memset(dmb, 0, sizeof *dmb);
	(void)memset(fld, 0, sizeof *fld);

	sep = strchr(line + 1, ':');
	val = sep + 1;
	while (*val == ' ' || *val == '\t')
		val++;

	dmb->nam = strndup(line, sep - line);
	dmb->val = strdup(val);
	fld->nam = dmb->nam;
	fld->val = dmb->val;

	nam = dmb->nam;
	while (*nam) {
		*nam = tolower(*nam);
		nam++;
	}

	tmp = strchr(fld->val, '\n');
	*tmp = '\0';

	res = hpack_search(hp, &idx, fld->nam, fld->val);

	if (res == HPACK_RES_IDX) {
		/* not in the cache? insert it */
		fld->flg = HPACK_FLG_TYP_DYN;
		return;
	}

	if (res == HPACK_RES_NAM) {
		/* only the name is in the cache? use it */
		fld->flg = HPACK_FLG_TYP_DYN | HPACK_FLG_NAM_IDX;
		fld->nam_idx = idx;
		return;
	}

	fld->flg = HPACK_FLG_TYP_IDX;
	fld->idx = idx;
}

/* cashdumb */

int
main(void)
{
	struct hpack *hp;
	struct dumb_state stt;
	char *lineptr;
	size_t linelen;

	/* initialization */
	hp = hpack_encoder(TABLE_SIZE, -1, hpack_default_alloc);
	(void)memset(&stt, 0, sizeof stt);
	stt.fld = static_fld;
	stt.dmb = static_dmb;
	stt.idx_len = HPACK_STATIC;
	lineptr = NULL;
	linelen = 0;

	/* repl */
	while (getline(&lineptr, &linelen, stdin) != -1) {
		if (*lineptr == '#') {
			LOG("%s", lineptr);
			continue;
		}

		if (*lineptr == '\n') {
			if (stt.len == 0)
				continue;
			LOG("encoding block\n");
			dumb_fields_send(hp, &stt, 0);
		}
		else {
			if (stt.len == MAX_FIELDS) {
				LOG("encoding partial block\n");
				dumb_fields_send(hp, &stt, 1);
			}
			dumb_fields_append(hp, &stt, lineptr);
		}
	}

	if (stt.len > 0) {
		LOG("encoding final block\n");
		dumb_fields_send(hp, &stt, 0);
	}

	hpack_free(&hp);
	free(lineptr);

	return (0);
}
