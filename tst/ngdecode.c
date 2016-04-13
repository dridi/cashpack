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
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <nghttp2/nghttp2.h>

#include "tst.h"

static void
print_headers(nghttp2_hd_inflater *inf, void *buf, size_t len)
{
	uint8_t *in;
	nghttp2_nv nv;
	ssize_t rv;
	int flg;

	in = buf;

	while (len > 0) {

		flg = 0;
		rv = nghttp2_hd_inflate_hd(inf, &nv, &flg, in, len, 1);
		if (rv < 0) {
			ERR("nghttp2: error %zd (%s) at %zd (0x%02x)\n",
			    rv, nghttp2_strerror(rv), (void *)in - buf, *in);
			abort();
		}
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

	rv = nghttp2_hd_inflate_hd(inf, &nv, &flg, in, len, 1);
	assert(rv == 0);
	assert(~flg & NGHTTP2_HD_INFLATE_EMIT);
	assert(flg & NGHTTP2_HD_INFLATE_FINAL);
	(void)nghttp2_hd_inflate_end_headers(inf);
}

static void
print_entries(nghttp2_hd_inflater *inf)
{
	char str[sizeof "\n[IDX] (s = LEN) "];
	const nghttp2_nv *nv;
	size_t idx, len, sz;
	int l;

	sz = 0;
	idx = 62;

	while (1) {
		nv = nghttp2_hd_inflate_get_table_entry(inf, idx);
		if (nv == NULL)
			break;

		if (sz == 0)
			OUT("\n");

		len = 32 + nv->namelen + nv->valuelen;
		l = snprintf(str, sizeof str, "\n[%3zu] (s = %3zu) ",
		    idx - 61, len);
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

int
main(int argc, char **argv)
{
	nghttp2_hd_inflater *inf;
	struct stat st;
	void *buf;
	int fd, retval, tbl_sz;

	tbl_sz = 4096; /* RFC 7540 Section 6.5.2 */

	/* ignore the command name */
	argc--;
	argv++;

	/* handle options */
	if (argc > 0 && !strcmp("-r", *argv)) {
		(void)fprintf(stderr, "Option -r not supported with ngdecode");
		abort();
	}

	if (argc > 0 && !strcmp("-t", *argv)) {
		assert(argc > 2);
		tbl_sz = atoi(argv[1]);
		assert(tbl_sz > 0);
		argc -= 2;
		argv += 2;
	}

	/* exactly one file name is expected */
	if (argc != 1) {
		fprintf(stderr, "Usage: ngdecode [-r <expected result>] "
		    "[-t <table size>] <dump file>\n\n"
		    "The file contains a dump of HPACK octets.\n"
		    "Default table size: 4096\n"
		    "Expected results: not supported\n");

		return (EXIT_FAILURE);
	}

	fd = open(*argv, O_RDONLY);
	assert(fd > STDERR_FILENO);

	retval = fstat(fd, &st);
	assert(retval == 0);
	(void)retval;

	buf = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	assert(buf != MAP_FAILED);

	retval = nghttp2_hd_inflate_new(&inf);
	assert(retval == 0);
	assert(inf != NULL);

	retval = nghttp2_hd_inflate_change_table_size(inf, tbl_sz);
	assert(retval == 0);

	/* NB: nghttp2 assumes that reducing the maximum table size below the
	 * current limit should be followed first thing by a dynamic table
	 * update during decoding. Whether that assumption is correct or not,
	 * this is purely an HTTP/2 concern and it shouldn't apply for the
	 * very first request when the initial size of the table may be set
	 * in the very settings frame. So we clear this state by pretending we
	 * just ended a request.
	 */
	(void)nghttp2_hd_inflate_end_headers(inf);

	OUT("Decoded header list:\n");

	print_headers(inf, buf, st.st_size);

	OUT("\n\nDynamic Table (after decoding):");

	print_entries(inf);

	nghttp2_hd_inflate_del(inf);

	retval = munmap(buf, st.st_size);
	assert(retval == 0);

	retval = close(fd);
	assert(retval == 0);

	return (EXIT_SUCCESS);
}
