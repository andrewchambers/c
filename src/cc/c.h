
/* Token types */
enum {
	TOKNUM = 256,
	TOKIDENT,
	TOKIF,
	TOKDO,
	TOKFOR,
	TOKWHILE,
	TOKRETURN,
	TOKINC,
	TOKDEC,
	TOKADDASS,
	TOKSUBASS,
	TOKMULASS,
	TOKDIVASS,
	TOKMODASS,
	TOKORASS,
	TOKANDASS,
	TOKEQL,
	TOKVOID,
	TOKCHAR,
	TOKINT,
	TOKELSE,
	TOKVOLATILE,
	TOKCONST,
	TOKLOR,
	TOKLAND,
	TOKNEQ,
	TOKLEQ,
	TOKGEQ,
	TOKSHL,
	TOKSHR,
	TOKARROW,
	TOKSTR,
	TOKTYPEDEF,
	TOKSIGNED,
	TOKUNSIGNED,
	TOKSHORT,
	TOKLONG,
	TOKSIZEOF,
	TOKFLOAT,
	TOKDOUBLE,
	TOKSTRUCT,
	TOKUNION,
	TOKGOTO,
	TOKSWITCH,
	TOKCASE,
	TOKDEFAULT,
	TOKCONTINUE,
	TOKBREAK,
	TOKREGISTER,
	TOKEXTERN,
	TOKSTATIC,
	TOKAUTO,
	TOKENUM,
	TOKELLIPSIS,
	TOKEOF, /* EOF needs to be the last. */
};

/* Storage classes */
enum {
	SCNONE,
	SCEXTERN,
	SCSTATIC,
	SCREGISTER,
	SCGLOBAL,
	SCTYPEDEF,
	SCAUTO,
};

typedef struct SrcPos SrcPos;
struct SrcPos {
	char *file;
	int   line;
	int   col;
};

enum {
	CVOID,
	CPRIM,
	CSTRUCT,
	CPTR,
	CFUNC,
};

enum {
	PRIMENUM,
	PRIMCHAR,
	PRIMSHORT,
	PRIMINT,
	PRIMLONG,
	PRIMLLONG,
	PRIMFLOAT,
	PRIMDOUBLE,
	PRIMLDOUBLE,
};

typedef struct StructMember StructMember;
typedef struct NameTy NameTy;
typedef struct CTy CTy;

struct StructMember {
	char *name;
	CTy  *type;
};

struct NameTy {
	char *name;
	CTy  *type;
};

struct CTy {
	/* type tag, one of the C* types */
	int t;
	union {
		struct {
			CTy  *rtype;
			List *params;
			int isvararg;
		} Func;
		struct {
			int isunion;
			List *members;
		} Struct;
		struct {
			CTy *subty;
		} Ptr;
		struct {
			CTy *subty;
		} Arr;
		struct {
			int issigned;
			int type;
		} Prim;
	};
};

typedef struct {
	SrcPos *pos;
	int     sclass;
	CTy    *type;
	char   *name;
	char   *label;  /* SCGLOBAL, SCSTATIC. */
	int     offset; /* SCAUTO only. */
} Sym;

enum {
	NFUNC,
	NLABELED,
	NWHILE,
	NDOWHILE,
	NFOR,
	NBINOP,
	NBLOCK,
	NUNOP,
	NCAST,
	NCASE,
	NCOMMA,
	NINIT,
	NRETURN,
	NSWITCH,
	NGOTO,
	NIDENT,
	NNUM,
	NSTR,
	NIDX,
	NSEL,
	NCALL,
	NSIZEOF,
	NIF,
	NDECL,
	NEXPRSTMT,
};

typedef struct Node Node;
struct Node {
	/* type tag, one of the N* types */
	int t;
	SrcPos pos;
	CTy *type;
	union {
		struct {
			int   localsz;
			char *name;
			Node *body;
		} Func;
		struct {
			int sclass;
			Vec *syms;
		} Decl;
		struct {
			char *lelse;
			Node *expr;
			Node *iftrue;
			Node *iffalse;
		} If;
		struct {
			char *lstart;
			char *lend;
			Node *init;
			Node *cond;
			Node *step;
			Node *stmt;
		} For;
		struct {
			char *lstart;
			char *lcond;
			char *lend;
			Node *stmt;
			Node *expr;
		} DoWhile;
		struct {
			char *lstart;
			char *lend;
			Node *expr;
			Node *stmt;
		} While;
		struct {
			char *lend;
			char *ldefault;
			Node *expr;
			Node *stmt;
			Vec  *cases;
		} Switch;
		struct {
			char *l;
			Node *expr;
			Node *stmt;
		} Case;
		struct {
			char *l;
			Node *stmt;
		} Labeled;
		struct {
			Node *expr;
		} Return;
		struct {
			Vec *exprs;
		} Comma;
		struct {
			Vec *stmts;
		} Block;
		struct {
			int op;
			Node *l;
			Node *r;
		} Binop;
		struct {
			int   op;
			Node *operand;
		} Unop;
		struct {
			Node *operand;
		} Cast;
		struct {
			List *inits;
		} Init;
		struct {
			/* TODO: parse to int */
			char *v;
		} Num;
		struct {
			char *v;
		} Str;
		struct {
			char *l;
			char *name;
		} Goto;
		struct {
			Sym  *sym;
		} Ident;
		struct {
			Node *idx;
			Node *operand;
		} Idx;
		struct {
			int arrow;
			char *sel;
			Node *operand;
		} Sel;
		struct {
			Node *expr;
		} ExprStmt;
		struct {
			Node *funclike;
			List *args;
		} Call;
		struct {
			CTy *t;
		} Sizeof;
	};
};

#define MAXTOKSZ 4096

typedef struct Lexer Lexer;
struct Lexer {
	FILE *f;
	SrcPos pos;
	SrcPos prevpos;
	SrcPos markpos;
	int  nchars;
	char tokval[MAXTOKSZ+1];
};

typedef struct Tok Tok;
struct Tok {
	int   k;
	char *v;
	/* Set if the tok was preceeded by an unescaped newline */
	int newline;
	SrcPos pos;
};

/* helper functions */
void errorf(char *, ...);
void errorposf(SrcPos *, char *, ...);
/* cpp functions */
void  cppinit(char *);
char *tokktostr(int);
Tok  *lex(Lexer *);
Tok  *pp(void);
/* type functions */
int isftype(CTy *);
int isitype(CTy *);
int isarithtype(CTy *);
int isptr(CTy *);
int tysize(CTy *);
int sametype(CTy *, CTy *);
/* parser functions */
void  parseinit(void);
Node *parsenext(void);
char *newlabel();
/* backend functions */
void  emitinit(FILE *);
void  emit(void);


