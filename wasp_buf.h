#ifndef WASP_BUF_H
#define WASP_BUF_H

#include <stddef.h>
#include <stdio.h>

typedef struct wasp_buf wasp_buf;
struct wasp_buf
{
	unsigned char *data;
	size_t size;
	size_t capacity;
};

void wasp_buf_init(wasp_buf *);

int wasp_buf_write(wasp_buf *, FILE *);

void wasp_buf_free(wasp_buf *);

#endif
