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

#include <hpack.h>

/* utility functions */

#define LOG(args...) fprintf(stderr, args)

static void
print_error(const char *func, int retval)
{

	LOG("%s: %s\n", func, hpack_strerror(retval));
	exit(EXIT_FAILURE);
}

/* index management */

#define TABLE_SIZE 256
#define MAX_ENTRIES (HPACK_STATIC + TABLE_SIZE / 33)

struct dumb_ref {
	const char *nam;
	const char *val;
};

struct dumb_ref dumb_idx[MAX_ENTRIES + 1];
size_t idx_len;

static void
dumb_index(enum hpack_event_e evt, const char *buf, size_t len, void *priv)
{

	(void)priv;
	(void)len;

	switch (evt) {
	case HPACK_EVT_FIELD:
		idx_len += 1;
		break;
	case HPACK_EVT_NAME:
		dumb_idx[idx_len].nam = buf;
		break;
	case HPACK_EVT_VALUE:
		dumb_idx[idx_len].val = buf;
		/* fall through */
	default:
		/* ignore other events */
		break;
	}
}

static size_t
search_index(const char *nam, const char *val)
{
	size_t nam_idx, idx;

	nam_idx = 0;

	for (idx = 1; idx <= idx_len; idx++)
		if (!strcmp(nam, dumb_idx[idx].nam)) {
			nam_idx = idx;
			if (!strcmp(val, dumb_idx[idx].val))
				return (idx);
		}

	return (nam_idx);
}

/* encoding logic */

#define MAX_FIELDS 10

struct dumb_state {
	struct hpack_field	*fld;
	size_t			off;
	ssize_t			max;
};

static void
dumb_encoding(enum hpack_event_e evt, const char *buf, size_t len, void *priv)
{
	struct hpack_field *hf;
	struct dumb_state *ds;

	ds = priv;
	hf = ds->fld;

	switch (evt) {
	case HPACK_EVT_FIELD:
		ds->fld++;
		LOG("encoding field: %s: %s ", hf->nam, hf->val);
		if (hf->flg & HPACK_FLG_TYP_IDX) {
			LOG("(indexed");
			if (hf->idx > ds->max) {
				LOG(", expired");
				hf->flg = HPACK_FLG_TYP_DYN;
				hf->idx = 0;
			}
			if (hf->idx > HPACK_STATIC)
				hf->idx += ds->off;
			if (hf->idx > 0)
				LOG(", %hu", hf->idx);
			LOG(")");
		}
		if (hf->flg & HPACK_FLG_NAM_IDX) {
			LOG("(indexed name");
			if (hf->nam_idx > ds->max) {
				LOG(", expired");
				hf->flg = HPACK_FLG_TYP_DYN;
				hf->nam_idx = 0;
			}
			if (hf->nam_idx > HPACK_STATIC)
				hf->nam_idx += ds->off;
			if (hf->nam_idx > 0)
				LOG(", %hu", hf->nam_idx);
			LOG(")");
		}
		LOG("\n");
		break;
	case HPACK_EVT_EVICT:
		LOG("eviction\n");
		ds->max--;
		break;
	case HPACK_EVT_INDEX:
		LOG("field indexed\n");
		ds->off++;
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
send_fields(struct hpack *hp, struct hpack_field *fld, size_t n_fld,
    unsigned cut)
{
	struct hpack_encoding enc;
	struct dumb_state state;
	char buf[256];
	int retval;

	state.fld = fld;
	state.off = 0;
	state.max = idx_len;

	enc.fld = fld;
	enc.fld_cnt = n_fld;
	enc.buf = buf;
	enc.buf_len = sizeof buf;
	enc.cb = dumb_encoding;
	enc.priv = &state;

	retval = hpack_encode(hp, &enc, cut);
	if (retval < 0)
		print_error("hpack_encode", retval);

	while (n_fld > 0) {
		/* clear the name and value that are always set */
		free(fld->nam);
		free(fld->val);
		fld->nam = NULL;
		fld->val = NULL;
		/* clear the rest of the field */
		retval = hpack_clean_field(fld);
		if (retval < 0)
			print_error("hpack_clean_field", retval);
		fld++;
		n_fld--;
	}
}

static void
make_field(struct hpack_field *fld, const char *line)
{
	const char *sep, *val;
	char *nam, *tmp;
	size_t idx;

	(void)memset(fld, 0, sizeof *fld);

	sep = strchr(line + 1, ':');
	val = sep + 1;
	while (*val == ' ' || *val == '\t')
		val++;

	fld->nam = strndup(line, sep - line);
	fld->val = strdup(val);

	nam = fld->nam;
	while (*nam) {
		*nam = tolower(*nam);
		nam++;
	}

	tmp = strchr(fld->val, '\n');
	*tmp = '\0';

	idx = search_index(fld->nam, fld->val);

	if (idx == 0) {
		/* not in the cache? insert it */
		fld->flg = HPACK_FLG_TYP_DYN;
		return;
	}

	if (strcmp(fld->val, dumb_idx[idx].val)) {
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
main(int argc, const char **argv)
{
	struct hpack *hp;
	struct hpack_field fld[MAX_FIELDS];
	char *lineptr;
	size_t linelen, n_fld;
	int retval;

	/* command-line arguments are not used */
	(void)argc;
	(void)argv;

	/* index initialization */
	idx_len = 0;
	retval = hpack_static(dumb_index, NULL);
	if (retval < 0)
		print_error("hpack_static", retval);

	/* remaining initialization */
	hp = hpack_encoder(TABLE_SIZE, -1, hpack_default_alloc);
	n_fld = 0;
	lineptr = NULL;
	linelen = 0;

	while (getline(&lineptr, &linelen, stdin) != -1) {
		if (*lineptr == '#') {
			LOG("%s", lineptr);
			continue;
		}

		if (*lineptr == '\n') {
			if (n_fld == 0)
				continue;
			LOG("encoding block\n");
			send_fields(hp, fld, n_fld, 0);
			n_fld = 0;
			idx_len = 0;
			retval = hpack_tables(hp, dumb_index, NULL);
			if (retval < 0)
				print_error("hpack_tables", retval);
			LOG("index length: %zu\n\n", idx_len);
		}
		else {
			if (n_fld == MAX_FIELDS) {
				LOG("encoding partial block\n");
				send_fields(hp, fld, n_fld, 1);
				n_fld = 0;
			}
			make_field(fld + n_fld, lineptr);
			n_fld++;
		}
	}

	if (n_fld > 0)
		send_fields(hp, fld, n_fld, 0);

	hpack_free(&hp);
	free(lineptr);

	return (0);
}
