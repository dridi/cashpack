/*-
 * Written by Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 *
 * This file is in the public domain.
 */

#include <stdio.h>

#define DBG(fmt, args...)				\
	do {						\
		fprintf(stderr, "%s(%d): " fmt "\n",	\
		    __func__, __LINE__, args);		\
	} while (0)
