
typedef enum {
	IRConst,
	IRLabel,
	IRVReg,
	IRNamedVReg
} IRValKind;

typedef struct IRVal {
	IRValKind kind;
	char  *irtype;
	union {
		int64 v;
		char  *label;
	};
} IRVal;

typedef enum {
	Opalloca,
	Opret,
	Opjmp,
	Opcall,
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
	Opcnel,
	Opcsltl,
	Opcsgtl,
	Opcsgel,
	Opcslel,
	Opceqw,
	Opcnew,
	Opcsltw,
	Opcsgtw,
	Opcsgew,
	Opcslew,
	Opload,
	Oploadsh,
	Oploadsb,
	Oploaduh,
	Oploadub,
	Opstorel,
	Opstorew,
	Opstoreh,
	Opstoreb,
	Opextsw,
	Opextsh,
	Opextsb,
	Opextuw,
	Opextuh,
	Opextub,
} Opcode;

typedef struct Instruction {
	Opcode op;
	IRVal a, b, c;
	IRVal *args;
	int nargs;
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

