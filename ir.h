
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
	Opcond,
	Opadd,
	Opsub,
	Opmul,
	Opdiv,
	Oprem,
	Opband,
	Opbor,
	Opbxor,
	Opceql,
	Opceqw,
	Opload,
	Oploadsh,
	Oploadsb,
	Oploaduh,
	Oploadub,
	Opstorel,
	Opstorew,
	Opstoreh,
	Opstoreb
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

BasicBlock *mkbasicblock(char *label);
char       *bbgetlabel(BasicBlock *);
void        bbappend(BasicBlock *, Instruction);
void        bbterminate(BasicBlock *, Terminator);

void  setiroutput(FILE *);

