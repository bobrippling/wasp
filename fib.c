#include <stdio.h>

#include "as.h"

void generate_fib_function(wasp_buf *buf)
{
	wasp_label end = { 0 };
	wasp_label loop = { 0 };

#ifdef CALL_IN_REG
	asm_xchg(buf, ECX, EDI);
#else
	asm_mov_mem_reg(buf, ESP, 8, ECX);
	// 4 for 32-bit
#endif

	asm_mov_imm32(buf, EAX, 0);
	asm_mov_imm32(buf, EDX, 1);
	asm_test(buf, ECX, ECX);
	asm_jcc(buf, CC_Z, &end);

	bind_label(buf, &loop);
	asm_xchg(buf, EAX, EDX);
	asm_add(buf, EAX, EDX);
	asm_loop(buf, &loop);

	bind_label(buf, &end);
	asm_ret(buf);
}

int main()
{
	typedef int func_t() __attribute((cdecl));

	wasp_buf buf;
	int i, x;
	func_t *func;

	buf.data = alloc_exec_mem(1024);
	buf.size = 0;
	buf.capacity = 1024;

	generate_fib_function(&buf);

	func = (void *)buf.data;

	printf("Code: ");
	for (i = 0; i < buf.size; i++) {
		printf("%02x", buf.data[i]);
	}
	printf("\n");

	for (i = 0; i < 10; i++) {
#ifdef CALL_IN_REG
		x = (*func)(i);
#else
		x = (*func)(0,0,0,0,0,0,i);
#endif

		printf("fib(%d) = %d\n", i, x);
	}

	return 0;
}
