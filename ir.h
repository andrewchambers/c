
typedef enum {
	IRConst,
	IRLabel,
	IRVReg
} IRValKind;

typedef struct IRVal {
	IRValKind kind;
	int64 v;
	char  *irtype;
	char  *label;
} IRVal;

typedef enum {
	Opalloca,
	Opret,
	Opjmp,
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

void endcurbb(Terminator);
IRVal nextvreg();

BasicBlock *mkbasicblock();
void        bbappend(BasicBlock *bb, Instruction ins);

extern BasicBlock *preludebb;
extern BasicBlock *currentbb;
extern Vec        *basicblocks;