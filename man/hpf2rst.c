/*-
 * Written by Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 *
 * This file is in the public domain.
 */

#include <stdio.h>

int
main(int argc, const char **argv)
{

#define HPF(f, v, l)				\
	printf("``HPACK_FLG_%s``\n\n", #f);	\
	printf("%s\n\n", l);
#include "tbl/hpack_tbl.h"
#undef HPF

	(void)argc;
	(void)argv;
	return (0);
}
