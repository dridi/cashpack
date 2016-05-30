/*-
 * Written by Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 *
 * This file is in the public domain.
 */

#include <stdio.h>

int
main(int argc, const char **argv)
{

#define HPE(e, v, d, l) printf("``HPACK_EVT_%s`` (%s)\n\n%s", #e, d, l);
#include "tbl/hpack_tbl.h"
#undef HPE

	(void)argc;
	(void)argv;
	return (0);
}
