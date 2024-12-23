/*-
 * License: BSD-2-Clause
 * (c) 2017-2024 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 */

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "hpack.h"
#include "hpack_assert.h"

#include "tst.h"

struct fld_dec_priv {
	struct hpack	*hp;
	hpack_event_f	*cb;
	void		*buf;
	size_t		len;
	const char	*nam;
	const char	*val;
	unsigned	skp;
};

static int
decode_block(struct dec_ctx *ctx, const void *blk, size_t len, unsigned cut)
{
	struct fld_dec_priv *dp;
	struct hpack_decoding dec;
	int retval;

	dp = ctx->priv;
	dec.blk = blk;
	dec.blk_len = len;
	dec.buf = dp->buf;
	dec.buf_len = dp->len;
	dec.cb = dp->cb;
	dec.priv = NULL;
	dec.cut = cut;

	while ((retval = hpack_decode_fields(dp->hp, &dec, &dp->nam,
	    &dp->val)) == HPACK_RES_FLD)
		printf("\n%s: %s", dp->nam, dp->val);

	if (retval == HPACK_RES_OK)
		assert(!cut);

	if (retval == HPACK_RES_BLK) {
		assert(cut);
		if (!dp->skp)
			retval = HPACK_RES_OK;
	}

	return (retval);
}

static int
skip_block(struct dec_ctx *ctx, const void *blk, size_t len, unsigned cut)
{
	struct fld_dec_priv *dp;
	int retval;

	dp = ctx->priv;
	dp->skp = 1;
	retval = decode_block(ctx, blk, len, cut);
	dp->skp = 0;

	if (retval == HPACK_RES_BLK) {
		assert(cut);
		retval = HPACK_RES_OK;
	}
	else {
		assert(retval == HPACK_RES_SKP);
		OUT("\n<too big>");
		assert(!cut);
		retval = hpack_skip(dp->hp);
	}

	return (retval);
}

static int
resize_table(struct dec_ctx *ctx, const void *buf, size_t len, unsigned cut)
{
	struct fld_dec_priv *dp;

	(void)buf;
	(void)cut;
	dp = ctx->priv;
	assert(dp->skp == 0);
	return (hpack_resize(&dp->hp, len));
}

int
main(int argc, char **argv)
{
	enum hpack_result_e res, exp;
	struct dec_ctx ctx;
	struct fld_dec_priv priv;
	struct stat st;
	char buf[4096];
	void *blk;
	int fd, retval, tbl_sz;

	TST_signal();

	priv.buf = buf;
	priv.len = sizeof buf;
	priv.nam = NULL;
	priv.val = NULL;
	priv.skp = 0;

	ctx.dec = decode_block;
	ctx.skp = skip_block;
	ctx.rsz = resize_table;
	ctx.priv = &priv;
	ctx.spec = "";
	tbl_sz = 4096; /* RFC 7540 Section 6.5.2 */
	exp = HPACK_RES_OK;

	/* ignore the command name */
	argc--;
	argv++;

	/* handle options */
	if (argc > 0 && !strcmp("--buffer-size", *argv)) {
		assert(argc > 2);
		priv.len = atoi(argv[1]);
		assert(priv.len > 0);
		assert(priv.len < sizeof buf);
		argc -= 2;
		argv += 2;
	}

	if (argc > 0 && !strcmp("--decoding-spec", *argv)) {
		assert(argc > 2);
		ctx.spec = argv[1];
		argc -= 2;
		argv += 2;
	}

	if (argc > 0 && !strcmp("--expect-error", *argv)) {
		assert(argc > 2);
		exp = TST_translate_error(argv[1]);
		assert(exp < 0);
		argc -= 2;
		argv += 2;
	}

	if (argc > 0 && !strcmp("--table-size", *argv)) {
		assert(argc > 2);
		tbl_sz = atoi(argv[1]);
		assert(tbl_sz > 0);
		argc -= 2;
		argv += 2;
	}

	/* exactly one file name is expected */
	if (argc != 1) {
		fprintf(stderr, "Usage: fdecode [--expect-error <ERR>] "
		    "[--decoding-spec <spec>,[...]] [--table-size <size>] "
		    "[--buffer-size <size>] <dump file>\n\n"
		    "The file contains a dump of HPACK octets.\n\n"
		    "Spec format: <letter><size>\n"
		    "  a - abort the decoding process\n"
		    "  d - decode a block of <size> bytes from the dump\n"
		    "  p - decode a partial block of <size> bytes\n"
		    "  r - resize the dynamic table to <size> bytes\n"
		    "  s - try to decode <size> bytes and skip the rest\n"
		    "  S - the same as 's' but for partial blocks\n"
		    "  The last empty spec decodes the rest of the dump\n"
		    "Default table size: 4096\n"
		    "Possible errors:\n");

#define HPR_ERRORS_ONLY
#define HPR(val, cod, txt, rst) fprintf(stderr, "  %s: %s\n", #val, txt);
#include "tbl/hpack_tbl.h"
#undef HPR
#undef HPR_ERRORS_ONLY

		return (EXIT_FAILURE);
	}

	fd = open(*argv, O_RDONLY);
	assert(fd > STDERR_FILENO);

	retval = fstat(fd, &st);
	assert(retval == 0);
	ctx.blk_len = st.st_size;

#ifdef NDEBUG
	(void)retval;
#endif

	blk = malloc(st.st_size);
	assert(blk != NULL);

	retval = read(fd, blk, st.st_size);
	assert(retval == (int)st.st_size);

	retval = close(fd);
	assert(retval == 0);

	ctx.blk = blk;

	hp = hpack_decoder(tbl_sz, -1, hpack_default_alloc);
	assert(hp != NULL);

	priv.hp = hp;
	res = TST_decode(&ctx);

	OUT("\n\n");
	TST_print_table();

	hpack_free(&hp);
	free(blk);

	if (res != exp)
		ERR("hpack error: expected '%s' (%d) got '%s' (%d)",
		    hpack_strerror(exp), exp, hpack_strerror(res), res);

	return (res != exp);
}
