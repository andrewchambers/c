
typedef enum {
	Opalloca,
	Opret,
	Opjmp,
	Opcond,
	Opadd,
	Opsub,
	Opmul,
	Opdiv,
	Oprem,
	Opband,
	Opbor,
	Opbxor
} Opcode;

typedef struct Instruction {
	Opcode op;
	IRVal a, b, c;
} Instruction;

typedef struct Terminator {
	Opcode op;
	IRVal v;
	char *label1;
	char *label2;
} Terminator;

typedef struct BasicBlock {
	Vec *labels;
	Instruction *instructions;
	Terminator terminator;
	int cap;
	int ninstructions;
	int terminated;
} BasicBlock;

char *ctype2irtype(CTy *ty);
char *newlabel();
void  setiroutput(FILE *);
void  beginmodule();
void  emitsym(Sym *);
void  emitfuncstart(Sym *);
void  emitfuncend();
void  endmodule();

void setcurbb(BasicBlock *);
IRVal nextvreg();
IRVal alloclocal(CTy *ty);

BasicBlock *mkbasicblock();
char       *bbgetlabel(BasicBlock *);
void        bbappend(BasicBlock *, Instruction);
void        bbterminate(BasicBlock *, Terminator);

extern BasicBlock *preludebb;
extern BasicBlock *currentbb;
extern Vec        *basicblocks;