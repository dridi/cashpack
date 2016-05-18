/*-
 * Written by Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 *
 * This file is in the public domain.
 */

#include <stdio.h>

int
main(int argc, const char **argv)
{

#define HPR(r, v, d, l)					\
	printf("``HPACK_RES_%s`` (%s)\n\n", #r, d);	\
	printf("%s\n\n", l);
#include "tbl/hpack_tbl.h"
#undef HPR

	(void)argc;
	(void)argv;
	return (0);
}
