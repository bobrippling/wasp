#include <stdlib.h>

#include "fatal.h"

#include "mem.h"

void *xmalloc(size_t l)
{
	void *p = malloc(l);
	if(!p)
		fatal("malloc:");
	return p;
}
