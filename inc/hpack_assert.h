/*-
 * Written by Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 *
 * This file is in the public domain.
 */

#define WRONG(str)		\
	do {			\
		assert(!str);	\
	} while (0)

#define INCOMPL()	WRONG("Incomplete code")
