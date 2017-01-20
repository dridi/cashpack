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
print_error(const char *func, int retval)
{

	LOG("%s: %s\n", func, hpack_strerror(retval));
	exit(EXIT_FAILURE);
}

/* data structures */

struct dumb_ref {
	const char *nam;
	const char *val;
};

struct dumb_field {
	char	*nam;
	char	*val;
};

struct dumb_state {
	struct dumb_ref		*idx;
	struct dumb_field	*dmb;
	struct hpack_field	*fld;
	size_t			len;
	size_t			pos;
	size_t			idx_len;
	size_t			idx_off;
	size_t			idx_max;
};

/* global variables for convenience */

#define TABLE_SIZE 256
#define MAX_ENTRIES (HPACK_STATIC + TABLE_SIZE / 33)
#define MAX_FIELDS 10

static struct hpack_field static_fld[MAX_FIELDS];
static struct dumb_field  static_dmb[MAX_FIELDS];
static struct dumb_ref    static_idx[MAX_ENTRIES + 1];

/* index management */

static void
dumb_index(enum hpack_event_e evt, const char *buf, size_t len, void *priv)
{
	struct dumb_state *stt;

	stt = priv;
	(void)len;

	switch (evt) {
	case HPACK_EVT_FIELD:
		stt->idx_len += 1;
		break;
	case HPACK_EVT_NAME:
		stt->idx[stt->idx_len].nam = buf;
		break;
	case HPACK_EVT_VALUE:
		stt->idx[stt->idx_len].val = buf;
		/* fall through */
	default:
		/* ignore other events */
		break;
	}
}

static size_t
search_index(const struct dumb_state *stt, const char *nam, const char *val)
{
	size_t nam_idx, idx;

	nam_idx = 0;

	for (idx = 1; idx <= stt->idx_len; idx++)
		if (!strcmp(nam, stt->idx[idx].nam)) {
			nam_idx = idx;
			if (!strcmp(val, stt->idx[idx].val))
				return (idx);
		}

	return (nam_idx);
}

/* encoding logic */

static void
dumb_encoding(enum hpack_event_e evt, const char *buf, size_t len, void *priv)
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
			LOG("(indexed");
			if (hf->idx > stt->idx_max) {
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
			if (hf->idx > 0)
				LOG(", %hu", hf->idx);
			LOG(")");
		}
		if (hf->flg & HPACK_FLG_NAM_IDX) {
			LOG("(indexed name");
			if (hf->nam_idx > stt->idx_max) {
				LOG(", expired");
				hf->flg = HPACK_FLG_TYP_DYN;
				hf->nam_idx = 0;
			}
			else
				hf->nam = NULL;
			if (hf->nam_idx > HPACK_STATIC)
				hf->nam_idx += stt->idx_off;
			if (hf->nam_idx > 0)
				LOG(", %hu", hf->nam_idx);
			LOG(")");
		}
		LOG("\n");
		break;
	case HPACK_EVT_EVICT:
		LOG("eviction\n");
		stt->idx_max--;
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
clear_fields(struct dumb_state *stt)
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
			print_error("hpack_clean_field", retval);
		dmb++;
		fld++;
		stt->len--;
	}
}

static void
send_fields(struct hpack *hp, struct dumb_state *stt, unsigned cut)
{
	struct hpack_encoding enc;
	char buf[256];
	int retval;

	stt->pos = 0;
	stt->idx_off = 0;
	stt->idx_max = stt->idx_len;

	enc.fld = stt->fld;
	enc.fld_cnt = stt->len;
	enc.buf = buf;
	enc.buf_len = sizeof buf;
	enc.cb = dumb_encoding;
	enc.priv = stt;
	enc.cut = cut;

	retval = hpack_encode(hp, &enc);
	if (retval < 0)
		print_error("hpack_encode", retval);

	clear_fields(stt);

	if (stt->idx_off > 0 || stt->idx_max != stt->idx_len)
		stt->idx_len = 0;
}

static void
make_field(struct dumb_state *stt, const char *line)
{
	struct hpack_field *fld;
	struct dumb_field *dmb;
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

	idx = search_index(stt, fld->nam, fld->val);

	if (idx == 0) {
		/* not in the cache? insert it */
		fld->flg = HPACK_FLG_TYP_DYN;
		return;
	}

	if (strcmp(fld->val, stt->idx[idx].val)) {
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
	int retval;

	/* initialization */
	hp = hpack_encoder(TABLE_SIZE, -1, hpack_default_alloc);
	(void)memset(&stt, 0, sizeof stt);
	stt.idx = static_idx;
	stt.fld = static_fld;
	stt.dmb = static_dmb;
	lineptr = NULL;
	linelen = 0;

	/* index initialization */
	retval = hpack_static(dumb_index, &stt);
	if (retval < 0)
		print_error("hpack_static", retval);

	while (getline(&lineptr, &linelen, stdin) != -1) {
		if (*lineptr == '#') {
			LOG("%s", lineptr);
			continue;
		}

		if (*lineptr == '\n') {
			if (stt.len == 0)
				continue;
			LOG("encoding block\n");
			send_fields(hp, &stt, 0);
			stt.idx_len = 0;
			retval = hpack_tables(hp, dumb_index, &stt);
			if (retval < 0)
				print_error("hpack_tables", retval);
			LOG("index length: %zu\n\n", stt.idx_len);
		}
		else {
			if (stt.len == MAX_FIELDS) {
				LOG("encoding partial block\n");
				send_fields(hp, &stt, 1);
			}
			make_field(&stt, lineptr);
		}
	}

	if (stt.len > 0)
		send_fields(hp, &stt, 0);

	hpack_free(&hp);
	free(lineptr);

	return (0);
}
