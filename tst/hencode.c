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

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "hpack.h"
#include "hpack_assert.h"

#include "tst.h"

#define TOKCMP(line, token) strncmp(line, token " ", sizeof token)

#define TOK_ARGS(line, token) (line + sizeof token)

static void
write_data(void *priv, enum hpack_evt_e evt, const void *buf, size_t len)
{

#ifdef NDEBUG
	(void)priv;
	(void)evt;
#endif

	assert(priv == NULL);
	assert(evt != HPACK_EVT_FIELD);
	assert(evt != HPACK_EVT_NEVER);
	assert(evt != HPACK_EVT_NAME);
	assert(evt != HPACK_EVT_VALUE);
	assert(evt != HPACK_EVT_TABLE);

	if (buf != NULL)
		WRT(buf, len);
}

static void
parse_name(struct hpack_item *itm, const char **args)
{
	char *sp;

	if (!TOKCMP(*args, "str")) {
		*args = TOK_ARGS(*args, "str");
		sp = strchr(*args, ' ');
		assert(sp != NULL);
		itm->fld.nam = strndup(*args, sp - *args);
		*args = sp + 1;
	}
	else if (!TOKCMP(*args, "huf")) {
		*args = TOK_ARGS(*args, "huf");
		sp = strchr(*args, ' ');
		assert(sp != NULL);
		itm->fld.nam = strndup(*args, sp - *args);
		itm->fld.flg = HPACK_NAM;
		*args = sp + 1;
	}
	else if (!TOKCMP(*args, "idx")) {
		*args = TOK_ARGS(*args, "idx");
		sp = strchr(*args, ' ');
		assert(sp != NULL);
		itm->fld.idx = atoi(*args);
		itm->fld.flg = HPACK_IDX;
		*args = sp + 1;
	}
	else
		WRONG("Unknown token");

	return;
}

static void
parse_value(struct hpack_item *itm, const char **args)
{
	char *ln;

	if (!TOKCMP(*args, "str")) {
		*args = TOK_ARGS(*args, "str");
		ln = strchr(*args, '\n');
		assert(ln != NULL);
		itm->fld.val = strndup(*args, ln - *args);
		*args = ln + 1;
	}
	else if (!TOKCMP(*args, "huf")) {
		*args = TOK_ARGS(*args, "huf");
		ln = strchr(*args, '\n');
		assert(ln != NULL);
		itm->fld.val = strndup(*args, ln - *args);
		itm->fld.flg |= HPACK_VAL;
		*args = ln + 1;
	}
	else
		WRONG("Unknown token");

	return;
}

static int
parse_command(struct hpack_item *itm, char **lineptr, size_t *line_sz)
{
	const char *args;
	ssize_t len;

	len = getline(lineptr, line_sz, stdin);
	if (len == -1) {
		free(*lineptr);
		*lineptr = NULL;
		return (-1);
	}

	assert((size_t)len == strlen(*lineptr));

	if (!TOKCMP(*lineptr, "indexed")) {
		args = TOK_ARGS(*lineptr, "indexed");
		itm->idx = atoi(args);
		itm->typ = HPACK_INDEXED;
	}
	else if (!TOKCMP(*lineptr, "dynamic")) {
		args = TOK_ARGS(*lineptr, "dynamic");
		itm->typ = HPACK_DYNAMIC;
		parse_name(itm, &args);
		parse_value(itm, &args);
	}
	else if (!TOKCMP(*lineptr, "never")) {
		args = TOK_ARGS(*lineptr, "never");
		itm->typ = HPACK_NEVER;
		parse_name(itm, &args);
		parse_value(itm, &args);
	}
	else if (!TOKCMP(*lineptr, "literal")) {
		args = TOK_ARGS(*lineptr, "literal");
		itm->typ = HPACK_LITERAL;
		parse_name(itm, &args);
		parse_value(itm, &args);
	}
	else if (!TOKCMP(*lineptr, "update")) {
		args = TOK_ARGS(*lineptr, "update");
		itm->lim = atoi(args);
		itm->typ = HPACK_UPDATE;
	}
	else
		WRONG("Unknown token");

	return (0);
}

void
free_item(struct hpack_item *itm)
{

	switch (itm->typ) {
	case HPACK_INDEXED:
	case HPACK_UPDATE:
		break;
	case HPACK_DYNAMIC:
	case HPACK_LITERAL:
	case HPACK_NEVER:
		if (~itm->fld.flg & HPACK_IDX)
			free((char *)itm->fld.nam);
		free((char *)itm->fld.val);
		break;
	default:
		WRONG("Unknwon type");
	}
}

int
main(int argc, char **argv)
{
	enum hpack_res_e res, exp;
	hpack_encoded_f *cb;
	struct hpack *hp;
	struct hpack_item itm;
	char *line;
	size_t line_sz;
	int tbl_sz;

	tbl_sz = 4096; /* RFC 7540 Section 6.5.2 */
	exp = HPACK_RES_OK;
	cb = write_data;

	/* ignore the command name */
	argc--;
	argv++;

	/* handle options */
	if (argc > 0 && !strcmp("-r", *argv)) {
		assert(argc >= 2);
#define HPR_ERRORS_ONLY
#define HPR(val, cod, txt)			\
		if (!strcmp(argv[1], #val))	\
			exp = HPACK_RES_##val;
#include "tbl/hpack_tbl.h"
#undef HPR
#undef HPR_ERRORS_ONLY
		assert(exp != HPACK_RES_OK);
		argc -= 2;
		argv += 2;
	}

	if (argc > 0 && !strcmp("-t", *argv)) {
		assert(argc == 2);
		tbl_sz = atoi(argv[1]);
		assert(tbl_sz > 0);
		argc -= 2;
		argv += 2;
	}

	/* exactly one file name is expected */
	if (argc != 0) {
		fprintf(stderr, "Usage: hencode [-r <expected result>] "
		    "[-t <table size>]\n\n"
		    "Default result: OK\n"
		    "Default table size: 4096\n"
		    "Possible results:\n");

#define HPR_ERRORS_ONLY
#define HPR(val, cod, txt)		\
		fprintf(stderr, "  %s: %s\n", #val, txt);
#include "tbl/hpack_tbl.h"
#undef HPR
#undef HPR_ERRORS_ONLY

		return (EXIT_FAILURE);
	}

	hp = hpack_encoder(tbl_sz, hpack_default_alloc);
	assert(hp != NULL);

	memset(&itm, 0, sizeof itm);
	res = HPACK_RES_OK;
	line = NULL;
	line_sz = 0;

	do {
		if (parse_command(&itm, &line, &line_sz) != 0)
			break;
		res = hpack_encode(hp, &itm, 1, cb, NULL);
		free_item(&itm);
		hpack_clean_item(&itm);
	} while (res == HPACK_RES_OK);

	free(line);

	if (res == HPACK_RES_OK) {
		fclose(stdout);
		stdout = fdopen(3, "a");

		print_dynamic_table(hp);
	}

	hpack_free(&hp);

	return (res != exp);
}
