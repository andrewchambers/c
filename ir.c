#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "util.h"
#include "ctypes.h"
#include "ir.h"

BasicBlock *
mkbasicblock(char *label)
{
	BasicBlock *bb;

	bb = xmalloc(sizeof(BasicBlock));
	bb->labels = vec();
	vecappend(bb->labels, label);
	bb->cap = 16;
	bb->instructions = xmalloc(bb->cap * sizeof(Instruction));
	bb->terminated = 0;
	bb->ninstructions = 0;
	return bb;
}

void
bbappend(BasicBlock *bb, Instruction ins)
{
	Instruction *instrarray;

	if (bb->terminated)
		return;

	if (bb->cap == bb->ninstructions) {
		bb->cap += 64;
		instrarray = xmalloc(bb->cap * sizeof(Instruction));
		bb->instructions = instrarray;
		memcpy(instrarray, bb->instructions, bb->ninstructions * sizeof(Instruction));
	}

	bb->instructions[bb->ninstructions++] = ins;
}

char *
bbgetlabel(BasicBlock *bb)
{
	return vecget(bb->labels, 0);
}

void
bbterminate(BasicBlock *bb, Terminator term)
{
	if (bb->terminated)
		return;

	bb->terminator = term;
	bb->terminated = 1;
}