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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "hpack.h"

#define WRT(buf, len)				\
	do {					\
		write(STDOUT_FILENO, buf, len);	\
	} while (0)

#define OUT(str)	WRT(str, sizeof(str) - 1)

void
print_headers(void *priv, enum hpack_evt_e evt, const void *buf, size_t len)
{

	assert(priv == NULL);

	switch (evt) {
	case HPACK_EVT_FIELD:
		assert(buf == NULL);
		OUT("\n");
		break;
	case HPACK_EVT_NAME:
		assert(buf != NULL);
		assert(len > 0);
		WRT(buf, len);
		OUT(": ");
		break;
	case HPACK_EVT_VALUE:
		assert(buf != NULL);
		assert(len > 0);
		WRT(buf, len);
		break;
	default:
		fprintf(stderr, "Unknwon event: %d\n", evt);
		abort();
	}
}

int
main(int argc, char **argv)
{
	enum hpack_res_e res;
	struct hpack *hp;
	struct stat st;
	void *buf;
	int fd;

	assert(argc == 2);

	fd = open(argv[1], O_RDONLY);
	assert(fd > STDERR_FILENO);

	assert(fstat(fd, &st) == 0);

	buf = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	assert(buf != NULL);

	hp = HPACK_decoder(0);
	assert(hp != NULL);

	OUT("Decoded header list:\n");

	res = HPACK_decode(hp, buf, st.st_size, print_headers, NULL);
	assert(res != HPACK_RES_DEV);
	assert(res == HPACK_RES_OK);

	OUT("\n\nDynamic table (after decoding): empty.\n");

	HPACK_free(&hp);

	return (0);
}
