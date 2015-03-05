#ifndef AS_H
#define AS_H

#include <stdbool.h>

typedef struct wasp_label wasp_label;
struct wasp_label
{
	size_t target_addr;
	size_t instr_addr;
	bool has_target;
	bool has_instr;
};

unsigned char *alloc_exec_mem(size_t size);

void asm_ret(wasp_buf *buffer);

void asm_mov_imm32(wasp_buf *buffer, wasp_reg dst, int32_t value);
void asm_mov_mem_reg(wasp_buf *buffer, wasp_reg src, int32_t disp, wasp_reg dst);
void asm_xchg(wasp_buf *buffer, wasp_reg reg1, wasp_reg reg2);

void asm_test(wasp_buf *buffer, wasp_reg reg1, wasp_reg reg2);

void asm_add(wasp_buf *buffer, wasp_reg src, wasp_reg dst);
void asm_xor(wasp_buf *buffer, wasp_reg src, wasp_reg dst);

void asm_loop(wasp_buf *buffer, wasp_label *label);
void asm_jcc(wasp_buf *buffer, enum wasp_cc cc, wasp_label *label);

void bind_label(wasp_buf *buffer, wasp_label *label);

#endif
