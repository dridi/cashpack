/*-
 * Copyright (c) 2017-2020 Dridi Boukelmoune
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
 * Poor man's micro benchmark.
 */

#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "hpack.h"

#define WRONG(str)		\
	do {			\
		perror(str);	\
		abort();	\
	} while (0)

#define FIELD_MARKER { 0, 0, 0, NULL, NULL }
#define FIELD_ENTRY(n, v) { HPACK_FLG_TYP_DYN|HPACK_FLG_AUT_IDX, 0, 0, n, v }
#define FIELD_LOOP(it, tbl) for (it = tbl; it->nam != NULL; it++)

static struct hpack *hp;

static struct hpack_field static_entries[] = {
#define HPS(i, n, v) { 0, 0, 0, n, v },
#include "tbl/hpack_static.h"
#undef HPS
	FIELD_MARKER
};

static struct hpack_field dynamic_entries[] = {
	FIELD_ENTRY(":method", "POST"),
	FIELD_ENTRY(":authority", "cashpack"),
	FIELD_ENTRY("user-agent", __FILE__ " cashpack/" PACKAGE_VERSION),
	FIELD_ENTRY("accept", "text/html,application/xml;q=0.9,*/*;q=0.8"),
	FIELD_ENTRY("accept-language", "en-US,en;q=0.5"),
	FIELD_ENTRY("accept-encoding", "gzip, deflate, br"),
	FIELD_ENTRY("x-forwarded-for", "127.0.0.1"),
	FIELD_ENTRY("via", "1.1 varnish"),
	FIELD_ENTRY("connection", "close"),
	FIELD_ENTRY("x-requested-with", "XMLHttpRequest"),
	FIELD_ENTRY("cookie", "optim=577475764078049311730638146188083"),
	FIELD_ENTRY("cache-control", "no-cache, stale-while-revalidate=0"),
	FIELD_ENTRY("pragma", "no-cache"),
	FIELD_ENTRY("upgrade-insecure-requests", "1"),
	FIELD_ENTRY("dnt", "1"),
	FIELD_ENTRY("mime-version", "1.0"),
	FIELD_ENTRY("content-type", "multipart/mixed; boundary=CASHPACK"),
	FIELD_ENTRY("content-encoding", "gzip"),
	FIELD_ENTRY("transfer-encoding", "chunked"),
	FIELD_MARKER
};

static struct hpack_field unknown_entries[] = {
	FIELD_ENTRY(":verb", "PURGE"),
	FIELD_ENTRY(":host", "localhost"),
	FIELD_ENTRY("aaa", "underflow"),
	FIELD_ENTRY("browser", "cashpack/42"),
	FIELD_ENTRY("want", "application/json,*/*;q=0.1"),
	FIELD_ENTRY("want-language", "fr-FR,fr;q=0.5"),
	FIELD_ENTRY("want-encoding", "deflate"),
	FIELD_ENTRY("forwarded", "for=localhost"),
	FIELD_ENTRY("through", "2 polish"),
	FIELD_ENTRY("session", "keep-alive"),
	FIELD_ENTRY("requested", "with=jSONHttpRequest"),
	FIELD_ENTRY("biscuit", "optim=42"),
	FIELD_ENTRY("cache-center", "private"),
	FIELD_ENTRY("realis", "no-store"),
	FIELD_ENTRY("downgrade-secure-requests", "0"),
	FIELD_ENTRY("dt", "0"),
	FIELD_ENTRY("meme-version", "9001.0"),
	FIELD_ENTRY("content-kind", "multipart/parallel; boundary=world"),
	FIELD_ENTRY("content-conversion", "deflate"),
	FIELD_ENTRY("transfer-conversion", "identity"),
	FIELD_MARKER
};

static void
mbm_noop_cb(enum hpack_event_e evt, const char *buf, size_t size, void *priv)
{

	(void)evt;
	(void)buf;
	(void)size;
	(void)priv;
}

static void
mbm_setup(void)
{
	const struct hpack_field *hf;
	struct hpack_encoding he;
	char buf[64];

	hp = hpack_encoder(4096, -1, hpack_default_alloc, 0);
	if (hp == NULL)
		WRONG("hpack_encoder");

	he.fld = dynamic_entries;
	he.fld_cnt = 0;
	he.buf = buf;
	he.buf_len = sizeof buf;
	he.cb = mbm_noop_cb;
	he.priv = NULL;
	he.cut = 0;
	FIELD_LOOP(hf, dynamic_entries)
		he.fld_cnt++;

	if (hpack_encode(hp, &he) < 0)
		WRONG("hpack_encode");
}

static int
mbm_usage(void)
{
	struct hpack_field *hf;
	struct rusage ru;
	int status;

	if (wait(&status) == -1)
		WRONG("wait");

	if (getrusage(RUSAGE_CHILDREN, &ru) == -1)
		WRONG("rusage");

#ifndef __APPLE__
#define PRINT_RUSAGE_LONG(fld)	(void)printf(#fld "\t%ld\n", ru.ru_##fld)
#define PRINT_RUSAGE_TIME(fld) \
	(void)printf(#fld "\t%ld.%ld\n", \
	    ru.ru_##fld.tv_sec, ru.ru_##fld.tv_usec)
	PRINT_RUSAGE_TIME(utime);
	PRINT_RUSAGE_TIME(stime);
	PRINT_RUSAGE_LONG(maxrss);
	PRINT_RUSAGE_LONG(minflt);
	PRINT_RUSAGE_LONG(majflt);
	PRINT_RUSAGE_LONG(nvcsw);
	PRINT_RUSAGE_LONG(nivcsw);
#undef PRINT_RUSAGE_TIME
#undef PRINT_RUSAGE_LONG
#endif

	/* additional coverage */
	FIELD_LOOP(hf, dynamic_entries)
		if (hpack_clean_field(hf) < 0)
			WRONG("hpack_clean_field");

	return (status);
}

static void
mbm_bench(void)
{
	const struct hpack_field *hf;
	uint16_t idx = 0;

	FIELD_LOOP(hf, static_entries)
		if (hpack_search(hp, &idx, hf->nam, hf->val) < 0 || idx == 0)
			WRONG("static_entries");

	FIELD_LOOP(hf, dynamic_entries)
		if (hpack_search(hp, &idx, hf->nam, hf->val) < 0 || idx == 0)
			WRONG("dynamic_entries");

	FIELD_LOOP(hf, dynamic_entries)
		if (hpack_search(hp, &idx, hf->nam, NULL) < 0 || idx == 0)
			WRONG("dynamic_names");

	FIELD_LOOP(hf, unknown_entries)
		if (hpack_search(hp, &idx, hf->nam, hf->val) >= 0 || idx != 0)
			WRONG("unknown_entries");
}

int
main(void)
{
	pid_t pid;
	int i;

	mbm_setup();

	pid = fork();
	if (pid == -1)
		WRONG("fork");

	if (pid > 0)
		return(mbm_usage());

	for (i = 0; i < 1000000; i++)
		mbm_bench();
	return (EXIT_SUCCESS);
}
