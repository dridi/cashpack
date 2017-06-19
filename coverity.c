/*-
 * Written by Dridi Boukelmoune <dridi.boukelmoune@gmail.com>
 *
 * This file is in the public domain.
 */

#define NULL ((void *)0)

void *
__coverity_fnptr_hpack_alloc_malloc(size_t len, void *priv)
{
	int has_memory;

	__coverity_negative_sink__(len);
	if (has_memory)
		return (__coverity_alloc__(len));
	else
		return (NULL);
}

void *
__coverity_fnptr_hpack_alloc_realloc(void *ptr, size_t len, void *priv)
{
	int has_memory;

	__coverity_negative_sink__(len);
	if (len == 0)
		__coverity_panic__();

	if (has_memory) {
		__coverity_free__(ptr);
		return (__coverity_alloc__(len));
	}
	else
		return (NULL);
}

void
__coverity_fnptr_hpack_alloc_free(void *ptr, void *priv)
{

	__coverity_free__(ptr);
}
