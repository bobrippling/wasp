#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "fatal.h"

void fatal(const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vfprintf(stderr, fmt, l);
	va_end(l);

	if(*fmt && fmt[strlen(fmt) - 1] == ':')
		fprintf(stderr, " %s", strerror(errno));

	fputc('\n', stderr);

	exit(1);
}
