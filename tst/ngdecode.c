/*-
 * License: BSD-2-Clause
 * (c) 2016-2024 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
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

#include <nghttp2/nghttp2.h>

#include "hpack.h"

#include "tst.h"

static int
print_headers(nghttp2_hd_inflater *inf, const void *buf, size_t len)
{
	uint8_t *in;
	nghttp2_nv nv;
	ssize_t rv;
	int flg;

	in = (uint8_t *)(uintptr_t)buf;

	while (len > 0) {

		flg = 0;
		rv = nghttp2_hd_inflate_hd(inf, &nv, &flg, in, len, 1);
		if (rv < 0)
			return (rv);
		assert(rv > 0);
		assert(rv <= (ssize_t)len);

		if (flg & NGHTTP2_HD_INFLATE_EMIT) {
			OUT("\n");
			WRT(nv.name, nv.namelen);
			OUT(": ");
			WRT(nv.value, nv.valuelen);
		}

		in += rv;
		len -= rv;
	}

	return (0);
}

static void
print_entries(nghttp2_hd_inflater *inf)
{
	char str[sizeof "\n[IDX] (s = LEN) "];
	const nghttp2_nv *nv;
	size_t idx, len, sz;
	int l;

	sz = 0;
	idx = HPACK_STATIC + 1;

	while (1) {
		nv = nghttp2_hd_inflate_get_table_entry(inf, idx);
		if (nv == NULL)
			break;

		if (sz == 0)
			OUT("\n");

		len = HPACK_OVERHEAD + nv->namelen + nv->valuelen;
		l = snprintf(str, sizeof str, "\n[%3zu] (s = %3zu) ",
		    idx - HPACK_STATIC, len);
		assert(l + 1 == sizeof str);

		WRT(str, l);
		WRT(nv->name, nv->namelen);
		OUT(": ");
		WRT(nv->value, nv->valuelen);

		sz += len;
		idx++;
	}

	if (sz == 0)
		OUT(" empty.\n");
	else {
		l = snprintf(str, sizeof str, "%3zu\n", sz);
		OUT("\n      Table size: ");
		WRT(str, l);
	}
}

static int
decode_block(struct dec_ctx *ctx, const void *buf, size_t len, unsigned cut)
{
	nghttp2_hd_inflater *inf;
	nghttp2_nv nv;
	uint8_t *end;
	int rv, flg;

	inf = ctx->priv;
	rv = print_headers(inf, buf, len);

	if (rv == 0 && !cut) {
		end = (uint8_t *)(uintptr_t)buf + len;
		flg = 0;
		rv = nghttp2_hd_inflate_hd(inf, &nv, &flg, end, 0, 1);
		assert(rv == 0);
		assert(~flg & NGHTTP2_HD_INFLATE_EMIT);
		assert(flg & NGHTTP2_HD_INFLATE_FINAL);
		(void)nghttp2_hd_inflate_end_headers(inf);
	}

	return (rv);
}

static int
resize_table(struct dec_ctx *ctx, const void *buf, size_t len, unsigned cut)
{
	nghttp2_hd_inflater *inf;

	(void)buf;
	(void)cut;
	inf = ctx->priv;
	return (nghttp2_hd_inflate_change_table_size(inf, len));
}

int
main(int argc, char **argv)
{
	nghttp2_hd_inflater *inf;
	struct dec_ctx ctx;
	struct stat st;
	void *blk;
	int fd, retval, tbl_sz, res, exp;

	TST_signal();

	ctx.dec = decode_block;
	ctx.skp = NULL;
	ctx.rsz = resize_table;
	ctx.spec = "";
	tbl_sz = 4096; /* RFC 7540 Section 6.5.2 */
	exp = HPACK_RES_OK;
	inf = NULL;

	/* ignore the command name */
	argc--;
	argv++;

	/* handle options */
	if (argc > 0 && !strcmp("--decoding-spec", *argv)) {
		assert(argc >= 2);
		ctx.spec = argv[1];
		argc -= 2;
		argv += 2;
	}

	if (argc > 0 && !strcmp("--expect-error", *argv)) {
		assert(argc >= 2);
		exp = TST_translate_error(argv[1]);
		assert(exp < 0);
		/* override with nghttp2's generic HPACK error */
		exp = NGHTTP2_ERR_HEADER_COMP;
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
		fprintf(stderr, "Usage: ngdecode [--expect-error <ERR>] "
		    "[--decoding-spec <spec>,[...]] [--table-size <size>] "
		    "<dump file>\n\n"
		    "The file contains a dump of HPACK octets.\n"
		    "Spec format: <letter><size>\n"
		    "  a - abort the decoding process\n"
		    "  d - decode <size> bytes from the dump\n"
		    "  p - decode a partial block of <size> bytes\n"
		    "  r - resize the dynamic table to <size> bytes\n"
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

	retval = nghttp2_hd_inflate_new(&inf);
	assert(retval == 0);
	assert(inf != NULL);

	retval = nghttp2_hd_inflate_change_table_size(inf, tbl_sz);
	assert(retval == 0);

	/* NB: nghttp2 doesn't allow to pick an initial size for the dynamic
	 * table. So for tests starting with a reduced table, like some of the
	 * examples from the RFC, an update will be needed on the first block.
	 *
	 * We clear this state by pretending we just ended a request.
	 */
	(void)nghttp2_hd_inflate_end_headers(inf);

	ctx.priv = inf;
	res = TST_decode(&ctx);

	OUT("\n\nDynamic Table (after decoding):");

	print_entries(inf);

	nghttp2_hd_inflate_del(inf);
	free(blk);

	if (res != exp)
		ERR("nghttp2 result: %s (%d)", nghttp2_strerror(res), res);

	return (res != exp);
}
