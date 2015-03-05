#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "fatal.h"
#include "str.h"
#include "wasp_buf.h"

#include "x86_64.h"

enum wasp_cc
{
	CC_E = 0,
	CC_Z = 0x4,
};

typedef struct
{
	enum reg_idx
	{
		EAX = 0x0,
		ECX = 0x1,
		EDX = 0x2,
		EBX = 0x3,
		ESP = 0x4,
		EBP = 0x5,
		ESI = 0x6,
		EDI = 0x7,
	} idx;
	int size, high;
} reg;

struct x86_operand
{
	enum
	{
		operand_memreg,  /* -6(%rbp) */
		operand_memaddr, /* 4 */
		operand_reg,     /* %ecx */
		operand_literal /* $2 */
	} type;

	union
	{
		struct
		{
			reg reg;
			long offset;
		} memreg;
		long memaddr;
		reg reg;
		long literal;
	} u;
};

struct x86_isn
{
	enum isn
	{
		isn_ret,

		isn_mov,
		isn_xchg,
		isn_test,

		isn_loop,
		isn_jcc,
		isn_jmp,

		isn_setcc,

		isn_op,
	} type;

	struct x86_operand operands[2];

	union
	{
		struct
		{
			enum
			{
				op_add,
				op_xor,
				op_sub
			} kind;
		} op;
	} u;
};

static const struct
{
	const char *begin;
	enum isn isn;
	int noperands;
} ops[] = {
	{ "mov", isn_mov, 2 },
	{ "test", isn_test, 2 },
	{ "xchg", isn_test, 2 },
	{ 0 },
};

static long parse_num(char **const str)
{
	return strtol(*str, str, 0);
}

static void parse_reg(char **const pstr, reg *const out)
{
	static const struct
	{
		const char name[4];
		enum reg_idx idx;
		int size;
		int is_high;
	} regs[] = {

#define GREG(ch, i, post) \
		{ "r"ch post, i, 8 },   \
		{ "e"ch post, i, 4 },   \
		{  ""ch post, i, 2 }

#define QREG(ch, i)       \
		GREG(ch, i, "x"),     \
		{  ""ch"l", i, 1 },   \
		{  ""ch"h", i, 1, 1 }

#define IREG(ch, i)   \
		GREG(ch, i, "i"), \
		{ ch "il", i, 1 }

#define SREG(ch, i)   \
		GREG(ch, i, "p"), \
		{ ch "pl", i, 1 }

		QREG("a", EAX),
		QREG("b", EBX),
		QREG("c", ECX),
		QREG("d", EDX),

		SREG("s", ESP),
		SREG("b", EBP),

		IREG("s", ESI),
		IREG("d", EDI),

		{ 0 }

		/* r[8 - 15] -> r8b, r8w, r8d, r8 */
	};

	char *str = *pstr;
	char *end, save;
	int i;

	for(end = str; isalnum(*end); end++);

	save = *end;
	*end = '\0';

	for(i = 0; regs[i].name[0]; i++){
		if(!strcmp(regs[i].name, str)){
			out->idx = regs[i].idx;
			out->size = regs[i].size;
			out->high = regs[i].is_high;
			break;
		}
	}

	if(!regs[i].name[0])
		fatal("unknown register \"%s\"", str);

	*end = save;
	*pstr = end;
}

static void parse_operand(char **const str, struct x86_operand *const op)
{
	if(isdigit(**str))
		goto number;

	switch(**str){
number:
		case '-':
			op->type = operand_memaddr;
			op->u.memaddr = parse_num(str);

			if(**str == '('){
				op->type = operand_memreg;
				op->u.memreg.offset = op->u.memaddr;
				++*str;
				parse_reg(str, &op->u.memreg.reg);
				if(**str != ')')
					fatal("')' expected in '%s'", *str);
			}
			break;

		case '(':
			/* TODO */
			fatal("TODO: '('");

		case '$':
			/* TODO: expression parsing */
			op->type = operand_literal;
			++*str;
			op->u.literal = parse_num(str);
			break;

		case '%':
			op->type = operand_reg;
			++*str;
			parse_reg(str, &op->u.reg);
			break;
	}
}

static void parse_isn(char *isn, struct x86_isn *const outisn)
{
	char *end;
	char *suffix = isn;
	char save;
	unsigned suffixlen;
	int i;
	int noperands;

	for(; isalnum(*suffix); suffix++);
	save = *suffix;
	*suffix = '\0';

	for(i = 0; ops[i].begin; i++)
		if(startswith(isn, ops[i].begin))
			break;

	if(!ops[i].begin)
		fatal("unknown instruction '%s'", isn);
	noperands = ops[i].noperands;

	/* isn suffix */
	*suffix = save;
	for(end = suffix; !isspace(*end); end++);

	suffixlen = end - suffix + 1;
	if(suffixlen > 1)
		fatal("bad suffix '%s' in '%s'", suffix, isn);

	outisn->type = ops[i].isn;

	for(i = 0; ; i++){
		struct x86_operand *op = &outisn->operands[i];
		end = skipspace(end);

		parse_operand(&end, op);
		if(i == noperands - 1)
			break;

		if(*end != ',')
			fatal("comma expected in '%s'", end);
		end++;
	}

	end = skipspace(end);
	if(*end)
		fatal("extra characters: '%s'", end);
}

static void parse_line(const char *line, wasp_buf *const buf)
{
	struct x86_isn isn;

	line = skipspace(line);

	parse_isn(skipspace(line), &isn);

	printf("isn %s\n",
			(char*[]){
			"ret",
			"mov",
			"xchg",
			"test",
			"loop",
			"jcc",
			"jmp",
			"setcc",
			"op",
			}[isn.type]);
}

void assemble_file(FILE *fin, FILE *fout)
{
	wasp_buf buf;
	char line[256];

	wasp_buf_init(&buf);

	while(fgets(line, sizeof line, fin)){
		parse_line(line, &buf);
	}

	if(ferror(fin))
		fatal("read:");

	wasp_buf_write(&buf, fout);
	wasp_buf_free(&buf);
}
