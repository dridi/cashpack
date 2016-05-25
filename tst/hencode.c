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

#define LINECMP(line, token) strcmp(line, token "\n")

#define TOKCMP(line, token) strncmp(line, token " ", sizeof token)

#define TOK_ARGS(line, token) (line + sizeof token)

struct enc_ctx {
	struct hpack		*hp;
	hpack_encoded_f		*cb;
	struct hpack_item	*itm;
	size_t			cnt;
	char			*line;
	size_t			line_sz;
	enum hpack_res_e	res;
};

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

	if (buf != NULL)
		WRT(buf, len);
}

static void
free_item(struct hpack_item *itm)
{

	switch (itm->typ) {
	case HPACK_FLD_INDEXED:
		break;
	case HPACK_FLD_DYNAMIC:
	case HPACK_FLD_LITERAL:
	case HPACK_FLD_NEVER:
		if (~itm->fld.flg & HPACK_IDX)
			free((char *)itm->fld.nam);
		free((char *)itm->fld.val);
		break;
	default:
		WRONG("Unknwon type");
	}
}

static void
encode_message(struct enc_ctx *ctx)
{
	struct hpack_item *itm;

	if (ctx->cnt == 0)
		return;

	ctx->res = hpack_encode(ctx->hp, ctx->itm, ctx->cnt, ctx->cb, NULL);
	itm = ctx->itm;

	while (ctx->cnt > 0) {
		free_item(itm);
		hpack_clean_item(itm);
		itm++;
		ctx->cnt--;
	}

	free(ctx->itm);
	ctx->itm = NULL;
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
parse_commands(struct enc_ctx *ctx)
{
	struct hpack_item *itm;
	const char *args;
	ssize_t len;

	len = getline(&ctx->line, &ctx->line_sz, stdin);
	if (len == -1)
		return (-1);

	if (!LINECMP(ctx->line, "flush")) {
		encode_message(ctx);
		return (0);
	}
	else if (!LINECMP(ctx->line, "trim")) {
		assert(ctx->cnt == 0);
		ctx->res = hpack_trim(&ctx->hp);
		return (0);
	}
	else if (!TOKCMP(ctx->line, "resize")) {
		assert(ctx->cnt == 0);
		args = TOK_ARGS(ctx->line, "resize");
		len = atoi(args);
		ctx->res = hpack_resize(&ctx->hp, len);
		return (0);
	}
	else if (!TOKCMP(ctx->line, "update")) {
		args = TOK_ARGS(ctx->line, "update");
		len = atoi(args);
		ctx->res = hpack_limit(ctx->hp, len);
		return (0);
	}

	ctx->cnt++;

	ctx->itm = realloc(ctx->itm, ctx->cnt * sizeof *itm);
	assert(ctx->itm != NULL);

	itm = ctx->itm + ctx->cnt - 1;
	(void)memset(itm, 0, sizeof *itm);

	assert(len >= 0);

	if (!TOKCMP(ctx->line, "indexed")) {
		args = TOK_ARGS(ctx->line, "indexed");
		itm->idx = atoi(args);
		itm->typ = HPACK_FLD_INDEXED;
	}
	else if (!TOKCMP(ctx->line, "dynamic")) {
		args = TOK_ARGS(ctx->line, "dynamic");
		itm->typ = HPACK_FLD_DYNAMIC;
		parse_name(itm, &args);
		parse_value(itm, &args);
	}
	else if (!TOKCMP(ctx->line, "never")) {
		args = TOK_ARGS(ctx->line, "never");
		itm->typ = HPACK_FLD_NEVER;
		parse_name(itm, &args);
		parse_value(itm, &args);
	}
	else if (!TOKCMP(ctx->line, "literal")) {
		args = TOK_ARGS(ctx->line, "literal");
		itm->typ = HPACK_FLD_LITERAL;
		parse_name(itm, &args);
		parse_value(itm, &args);
	}
	else
		WRONG("Unknown token");

	return (parse_commands(ctx));
}

struct hpack *hp = NULL;

int
main(int argc, char **argv)
{
	enum hpack_res_e exp;
	struct enc_ctx ctx;
	int tbl_sz;

	TST_signal();

	(void)memset(&ctx, 0, sizeof ctx);

	tbl_sz = 4096; /* RFC 7540 Section 6.5.2 */
	exp = HPACK_RES_OK;
	ctx.cb = write_data;

	/* ignore the command name */
	argc--;
	argv++;

	/* handle options */
	if (argc > 0 && !strcmp("--expect-error", *argv)) {
		assert(argc >= 2);
		exp = TST_translate_error(argv[1]);
		assert(exp != HPACK_RES_OK);
		argc -= 2;
		argv += 2;
	}

	if (argc > 0 && !strcmp("--table-size", *argv)) {
		assert(argc == 2);
		tbl_sz = atoi(argv[1]);
		assert(tbl_sz > 0);
		argc -= 2;
		argv += 2;
	}

	/* exactly one file name is expected */
	if (argc != 0) {
		fprintf(stderr, "Usage: hencode [--expect-error <ERR>] "
		    "[--table-size <size>]\n\n"
		    "Default table size: 4096\n"
		    "Possible errors:\n");

#define HPR_ERRORS_ONLY
#define HPR(val, cod, txt, rst) fprintf(stderr, "  %s: %s\n", #val, txt);
#include "tbl/hpack_tbl.h"
#undef HPR
#undef HPR_ERRORS_ONLY

		return (EXIT_FAILURE);
	}

	ctx.hp = hpack_encoder(tbl_sz, hpack_default_alloc);
	assert(ctx.hp != NULL);

	hp = ctx.hp;

	ctx.res = HPACK_RES_OK;

	do {
		assert(ctx.res == HPACK_RES_OK);
	} while (parse_commands(&ctx) == 0);

	encode_message(&ctx);
	free(ctx.line);

	if (ctx.res == HPACK_RES_OK) {
		fclose(stdout);
		stdout = fdopen(3, "a");
		TST_print_table(ctx.hp);
	}

	hpack_free(&ctx.hp);

	if (ctx.res != HPACK_RES_OK)
		ERR("hpack error: %s (%d)", hpack_strerror(ctx.res), ctx.res);

	return (ctx.res != exp);
}
