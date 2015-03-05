#include <stdio.h>
#include <stdlib.h>

#include <assert.h>

#include <sys/mman.h>

#include "as.h"

/* Allocate size bytes of executable memory. */
unsigned char *alloc_exec_mem(size_t size)
{
	void *ptr;

	ptr = mmap(0, size, PROT_READ | PROT_WRITE | PROT_EXEC,
			MAP_PRIVATE | MAP_ANON, -1, 0);

	if (ptr == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}

	return ptr;
}

static void emit_byte(wasp_buf *buffer, int32_t byte)
{
	assert((uint32_t) byte <= UINT8_MAX);
	assert(buffer->size < buffer->capacity);

	buffer->data[buffer->size++] = (unsigned char) byte;
}

static void emit_int32(wasp_buf *buffer, int32_t value)
{
	/* Emit value in little-endian order. */
	emit_byte(buffer, value >> 0);
	emit_byte(buffer, value >> 8);
	emit_byte(buffer, value >> 16);
	emit_byte(buffer, value >> 24);
}

void asm_ret(wasp_buf *buffer)
{
	/* 1100 0011 */
	emit_byte(buffer, 0xC3);
}

void asm_xor(wasp_buf *buffer, wasp_reg src, wasp_reg dst)
{
	/* 0011 000 | w=1 | 11 | src(3) | dst(3) */
	emit_byte(buffer, 0x31);
	emit_byte(buffer, (0x3 << 6) | (src << 3) | dst);
}

void asm_mov_imm32(wasp_buf *buffer, wasp_reg dst, int32_t value)
{
	if (value == 0) {
		asm_xor(buffer, dst, dst);
		return;
	}

	/* 1011 | w=1 | dst(3) | imm32 */
	emit_byte(buffer, 0xB8 | dst);
	emit_int32(buffer, value);
}

void asm_mov_mem_reg(wasp_buf *buffer, wasp_reg src, int32_t disp, wasp_reg dst)
{
	unsigned char mod, reg, rm;

	/* 1000 101 | w=1 | mod(2) | reg(3) | rm(3) | [sib(8)] | [disp(8/32)] */
	emit_byte(buffer, 0x8B);

	if (disp == 0) {
		mod = 0x0;
	} else if (disp <= INT8_MAX) {
		mod = 0x1;
	} else {
		mod = 0x2;
	}

	rm = src;
	reg = dst;

	emit_byte(buffer, (mod << 6) | (reg << 3) | rm);

	if (src == ESP) {
		/* Emit SIB (Scaled Index Byte). */
		/* ss=00 | index=100 | base=esp(3) */
		emit_byte(buffer, (0x0 << 6) | (0x4 << 3) | ESP);
	}

	if (mod == 0x1) {
		emit_byte(buffer, (unsigned char)disp);
	} else if (mod == 0x2) {
		emit_int32(buffer, disp);
	}
}

void asm_test(wasp_buf *buffer, wasp_reg reg1, wasp_reg reg2)
{
	/* 1000 010 | w=1 | 11 | reg1(3) | reg2(3) */
	emit_byte(buffer, 0x85);
	emit_byte(buffer, (0x3 << 6) | (reg1 << 3) | reg2);
}

void asm_xchg(wasp_buf *buffer, wasp_reg reg1, wasp_reg reg2)
{
	if (reg1 == EAX) {
		/* 1001 | 0 | reg2 */
		emit_byte(buffer, (0x9 << 4) | (0x0 << 3) | reg2);
		return;
	}

	if (reg2 == EAX) {
		asm_xchg(buffer, reg2, reg1);
		return;
	}

	/* 1000 011 | w=1 | 11 | reg1(3) | reg2(3) */
	emit_byte(buffer, 0x87);
	emit_byte(buffer, (0x3 << 6) | (reg1 << 3) | reg2);
}

void asm_add(wasp_buf *buffer, wasp_reg src, wasp_reg dst)
{
	/* 0000 000 | w=1 | 11 | src(3) | dst(3) */
	emit_byte(buffer, 0x01);
	emit_byte(buffer, (0x3 << 6) | (src << 3) | dst);
}

void asm_loop(wasp_buf *buffer, wasp_label *label)
{
	/* 1110 0010 | displacement(8) */

	ssize_t my_addr = buffer->size;
	ssize_t disp = 0;

	if (label->has_target) {
		disp = label->target_addr - (my_addr + 2);
	} else {
		assert(!label->has_instr && "Label already used!");
		label->instr_addr = my_addr;
		label->has_instr = true;
	}

	assert(disp >= INT8_MIN && disp <= INT8_MAX);
	emit_byte(buffer, 0xe2);
	emit_byte(buffer, (unsigned char) disp);
}

void asm_jcc(wasp_buf *buffer, enum wasp_cc cc, wasp_label *label)
{
	/* 0000 1111 1000 | cc(4) | disp(32) */

	ssize_t my_addr = buffer->size;
	ssize_t disp = 0;

	if (label->has_target) {
		disp = label->target_addr - (my_addr + 6);
	} else {
		assert(!label->has_instr && "Label already used!");
		label->instr_addr = my_addr;
		label->has_instr = true;
	}

	assert(disp >= INT32_MIN && disp <= INT32_MAX);
	emit_byte(buffer, 0x0f);
	emit_byte(buffer, (0x8 << 4) | cc);
	emit_int32(buffer, (int32_t) disp);
}

/* Bind label to the current address and update any instruction that uses it. */
void bind_label(wasp_buf *buffer, wasp_label *label)
{
	ssize_t disp;
	ssize_t orig_buf_size;
	ssize_t addr;

	assert(!label->has_target && "Label already bound!");

	addr = buffer->size;
	label->target_addr = addr;
	label->has_target = true;

	if (label->has_instr) {
		orig_buf_size = buffer->size;

		/* Update the jump instruction with the displacement. */
		switch (buffer->data[label->instr_addr]) {
			case 0xe2: /* loop */
				disp = addr - (label->instr_addr + 2);
				assert(disp >= INT8_MIN && disp <= INT8_MAX);
				buffer->size = label->instr_addr + 1;
				emit_byte(buffer, (unsigned char) disp);
				break;
			case 0x0f: /* jcc */
				disp = addr - (label->instr_addr + 6);
				assert(disp >= INT32_MIN && disp <= INT32_MAX);
				buffer->size = label->instr_addr + 2;
				emit_int32(buffer, (int32_t) disp);
				break;
			default:
				assert(0 && "Binding label to unknown jump.");
		}

		buffer->size = orig_buf_size;
	}
}
