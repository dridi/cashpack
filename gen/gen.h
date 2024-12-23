/*-
 * License: BSD-2-Clause
 * (c) 2016 Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 */

#define GEN(fmt, ...) printf(fmt "\n", __VA_ARGS__)
#define OUT(str) GEN("%s", str)

#define GEN_HDR()						\
	do {							\
		OUT("/*");					\
		OUT(" * NB:  This file is machine generated, "	\
		    "DO NOT EDIT!");				\
		OUT(" */");					\
		OUT("");					\
	} while (0)
