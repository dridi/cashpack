/*-
 * Copyright (c) 2016-2020 Dridi Boukelmoune
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

#define TRUST_ME(p) ((void *)(uintptr_t)(p))

struct enc_ctx {
	hpack_event_f		*cb;
	struct hpack_field	*fld;
	size_t			cnt;
	char			*line;
	size_t			line_sz;
	unsigned		cut;
	enum hpack_result_e	res;
};

static void
write_data(enum hpack_event_e evt, const char *buf, size_t len, void *priv)
{

#ifdef NDEBUG
	(void)priv;
	(void)evt;
#endif

	assert(priv == NULL);
	assert(evt != HPACK_EVT_NEVER);
	assert(evt != HPACK_EVT_NAME);
	assert(evt != HPACK_EVT_VALUE);

	if (buf != NULL)
		WRT(buf, len);
}

static void
free_field(struct hpack_field *fld)
{

	switch (fld->flg & HPACK_FLG_TYP_MSK) {
	case HPACK_FLG_TYP_IDX:
		break;
	case HPACK_FLG_TYP_DYN:
	case HPACK_FLG_TYP_LIT:
	case HPACK_FLG_TYP_NVR:
		if (~fld->flg & HPACK_FLG_NAM_IDX)
			free(TRUST_ME(fld->nam));
		free(TRUST_ME(fld->val));
		break;
	default:
		WRONG("Unknwon type");
	}
}

static void
encode_message(struct enc_ctx *ctx)
{
	struct hpack_encoding enc;
	struct hpack_field *fld;
	char buf[256];

	if (ctx->cnt == 0)
		return;

	enc.fld = ctx->fld;
	enc.fld_cnt = ctx->cnt;
	enc.buf = buf;
	enc.buf_len = sizeof buf;
	enc.cb = ctx->cb;
	enc.priv = NULL;
	enc.cut = ctx->cut;

	ctx->res = hpack_encode(hp, &enc);
	fld = ctx->fld;

	while (ctx->cnt > 0) {
		free_field(fld);
		hpack_clean_field(fld);
		fld++;
		ctx->cnt--;
	}

	free(ctx->fld);
	ctx->fld = NULL;
}

static void
parse_name(struct hpack_field *fld, const char **args)
{
	char *sp;

	if (!TOKCMP(*args, "str")) {
		*args = TOK_ARGS(*args, "str");
		sp = strchr(*args, ' ');
		assert(sp != NULL);
		fld->nam = strndup(*args, sp - *args);
		*args = sp + 1;
	}
	else if (!TOKCMP(*args, "huf")) {
		*args = TOK_ARGS(*args, "huf");
		sp = strchr(*args, ' ');
		assert(sp != NULL);
		fld->nam = strndup(*args, sp - *args);
		fld->flg |= HPACK_FLG_NAM_HUF;
		*args = sp + 1;
	}
	else if (!TOKCMP(*args, "idx")) {
		*args = TOK_ARGS(*args, "idx");
		sp = strchr(*args, ' ');
		assert(sp != NULL);
		fld->nam_idx = atoi(*args);
		fld->flg |= HPACK_FLG_NAM_IDX;
		*args = sp + 1;
	}
	else
		WRONG("Unknown token");
}

static void
parse_value(struct hpack_field *fld, const char **args)
{
	char *ln;

	if (!TOKCMP(*args, "str")) {
		*args = TOK_ARGS(*args, "str");
		ln = strchr(*args, '\n');
		assert(ln != NULL);
		fld->val = strndup(*args, ln - *args);
		*args = ln + 1;
	}
	else if (!TOKCMP(*args, "huf")) {
		*args = TOK_ARGS(*args, "huf");
		ln = strchr(*args, '\n');
		assert(ln != NULL);
		fld->val = strndup(*args, ln - *args);
		fld->flg |= HPACK_FLG_VAL_HUF;
		*args = ln + 1;
	}
	else
		WRONG("Unknown token");
}

static int
parse_commands(struct enc_ctx *ctx)
{
	struct hpack_field *fld;
	const char *args;
	ssize_t len;

	ctx->cut = 0;

	len = getline(&ctx->line, &ctx->line_sz, stdin);
	if (len == -1)
		return (-1);

	if (!LINECMP(ctx->line, "abort")) {
		abort();
		return (-1);
	}
	else if (!LINECMP(ctx->line, "send")) {
		assert(ctx->cnt > 0);
		encode_message(ctx);
		return (0);
	}
	else if (!LINECMP(ctx->line, "push")) {
		assert(ctx->cnt > 0);
		ctx->cut = 1;
		encode_message(ctx);
		return (0);
	}
	else if (!LINECMP(ctx->line, "trim")) {
		assert(ctx->cnt == 0);
		ctx->res = hpack_trim(&hp);
		return (0);
	}
	else if (!TOKCMP(ctx->line, "resize")) {
		assert(ctx->cnt == 0);
		args = TOK_ARGS(ctx->line, "resize");
		len = atoi(args);
		ctx->res = hpack_resize(&hp, len);
		return (0);
	}
	else if (!TOKCMP(ctx->line, "update")) {
		args = TOK_ARGS(ctx->line, "update");
		len = atoi(args);
		ctx->res = hpack_limit(&hp, len);
		return (0);
	}

	ctx->cnt++;

	ctx->fld = realloc(ctx->fld, ctx->cnt * sizeof *fld);
	assert(ctx->fld != NULL);

	fld = ctx->fld + ctx->cnt - 1;
	(void)memset(fld, 0, sizeof *fld);

	assert(len >= 0);

	if (!TOKCMP(ctx->line, "indexed")) {
		args = TOK_ARGS(ctx->line, "indexed");
		fld->idx = atoi(args);
		fld->flg = HPACK_FLG_TYP_IDX;
	}
	else if (!TOKCMP(ctx->line, "dynamic")) {
		args = TOK_ARGS(ctx->line, "dynamic");
		fld->flg = HPACK_FLG_TYP_DYN;
		parse_name(fld, &args);
		parse_value(fld, &args);
	}
	else if (!TOKCMP(ctx->line, "never")) {
		args = TOK_ARGS(ctx->line, "never");
		fld->flg = HPACK_FLG_TYP_NVR;
		parse_name(fld, &args);
		parse_value(fld, &args);
	}
	else if (!TOKCMP(ctx->line, "literal")) {
		args = TOK_ARGS(ctx->line, "literal");
		fld->flg = HPACK_FLG_TYP_LIT;
		parse_name(fld, &args);
		parse_value(fld, &args);
	}
	else if (!LINECMP(ctx->line, "corrupt"))
		fld->flg = 0xff;
	else
		WRONG("Unknown token");

	return (parse_commands(ctx));
}

int
main(int argc, char **argv)
{
	enum hpack_result_e exp;
	struct enc_ctx ctx;
	int tbl_sz, tbl_mem, retval;

#ifdef NDEBUG
	(void)retval;
#endif

	TST_signal();

	(void)memset(&ctx, 0, sizeof ctx);

	tbl_sz = 4096; /* RFC 7540 Section 6.5.2 */
	tbl_mem = -1;
	exp = HPACK_RES_OK;
	ctx.cb = write_data;

	/* ignore the command name */
	argc--;
	argv++;

	/* handle options */
	if (argc > 0 && !strcmp("--expect-error", *argv)) {
		assert(argc >= 2);
		exp = TST_translate_error(argv[1]);
		assert(exp < 0);
		argc -= 2;
		argv += 2;
	}

	if (argc > 0 && !strcmp("--table-limit", *argv)) {
		assert(argc >= 2);
		tbl_mem = atoi(argv[1]);
		assert(tbl_mem > 0);
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

	/* hencode expects only options, no arguments */
	if (argc != 0) {
		fprintf(stderr, "Unexpected argument: %s\n\n"
		    "Usage: hencode [--expect-error <ERR>] "
		    "[--table-size <size>]\n\n"
		    "Default table size: 4096\n"
		    "Possible errors:\n",
		    *argv);

#define HPR_ERRORS_ONLY
#define HPR(val, cod, txt, rst) fprintf(stderr, "  %s: %s\n", #val, txt);
#include "tbl/hpack_tbl.h"
#undef HPR
#undef HPR_ERRORS_ONLY

		return (EXIT_FAILURE);
	}

	hp = hpack_encoder(tbl_sz, tbl_mem, hpack_default_alloc, 0);
	assert(hp != NULL);

	ctx.res = HPACK_RES_OK;

	do {
		assert(ctx.res == HPACK_RES_OK || ctx.res == HPACK_RES_BLK);
	} while (parse_commands(&ctx) == 0);

	encode_message(&ctx);
	free(ctx.line);

	if (ctx.res == HPACK_RES_OK) {
		retval = fflush(stdout);
		assert(retval == 0);

		retval = dup2(3, STDOUT_FILENO);
		assert(retval == STDOUT_FILENO);

		TST_print_table();

		retval = fclose(stdout);
		assert(retval == 0);
	}

	hpack_free(&hp);

	if (ctx.res != exp)
		ERR("hpack error: expected '%s' (%d) got '%s' (%d)",
		    hpack_strerror(exp), exp,
		    hpack_strerror(ctx.res), ctx.res);

	return (ctx.res != exp);
}
