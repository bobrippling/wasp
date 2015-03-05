#include <string.h>
#include <ctype.h>

#include "str.h"

char *skipspace(const char *s)
{
	for(; isspace(*s); s++);
	return (char *)s;
}

int startswith(const char *full, const char *prefix)
{
	return !strncmp(full, prefix, strlen(prefix));
}
