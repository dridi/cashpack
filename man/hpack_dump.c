/*-
 * License: BSD-2-Clause
 * (c) 2017 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 */

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <hpack.h>

static void
dump_cb(void *priv, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(priv, fmt, ap);
    va_end(ap);
}

int
main(void)
{
	struct hpack *hp;

	hp = hpack_encoder(4096, -1, hpack_default_alloc);
	if (hp == NULL)
		return (EXIT_FAILURE);

	hpack_dump(hp, dump_cb, stderr);
	hpack_free(&hp);

	return (EXIT_SUCCESS);
}
