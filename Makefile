include Makefile.cfg

CFLAGS = -g -Wall -Wextra -pedantic -std=c89 ${CFLAGS_CONFIGURE}

ARCH = x86_64
ARCH_DEF = -DWASP_ARCH_FNAME=\"${ARCH}.h\"

OBJ = main.o fatal.o mem.o wasp_buf.o str.o ${ARCH}.o

wasp: ${OBJ}
	${CC} -o $@ ${OBJ}

test: wasp
	echo 'mov %eax, %ebx' | ./wasp

main.o: main.c
	${CC} ${CFLAGS} -c -o $@ $< ${ARCH_DEF}

clean:
	rm -f ${OBJ} wasp

.PHONY: clean

fib.o: fib.c as.h
as.o: as.c as.h
