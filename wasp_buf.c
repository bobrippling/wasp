#include <stdlib.h>
#include <string.h>

#include "mem.h"

#include "wasp_buf.h"

void wasp_buf_init(wasp_buf *buf)
{
	buf->size = 0;
	buf->capacity = 512;
	buf->data = xmalloc(buf->capacity);
}

int wasp_buf_write(wasp_buf *buf, FILE *f)
{
	size_t nwrite = fwrite(buf->data, sizeof(char), buf->size, f);

	return nwrite == 0 ? 1 : 0;
}

void wasp_buf_free(wasp_buf *buf)
{
	free(buf->data);
	memset(buf, 0, sizeof *buf);
}
