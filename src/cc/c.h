/* Provides the preprocessor, compiler frontend. It also
   contains the interface the backends implement. */

typedef struct SrcPos SrcPos;
typedef struct StructMember StructMember;
typedef struct NameTy NameTy;
typedef struct CTy CTy;
typedef struct Sym Sym;
typedef struct Node Node;
typedef struct Lexer Lexer;

/* Token types */
enum Tokkind {
	TOKADD    = '+',
	TOKSUB    = '-',
	TOKMUL    = '*',
	TOKMOD    = '%',
	TOKDIV    = '/',
	TOKNOT    = '!',
	TOKBNOT   = '~',
	TOKGT     = '>',
	TOKLT     = '<',
	TOKASS    = '=',
	TOKQUES   = '?',
	TOKLBRACK = '[',
	TOKRBRACK = ']',
	TOKBAND   = '&',
	TOKBOR    = '|',
	TOKLBRACE = '{',
	TOKRBRACE = '}',
	TOKSEMI   = ';',
	TOKLPAREN = '(',
	TOKRPAREN = ')',
	TOKDOT    = '.',
	TOKCOLON  = ':',
	TOKXOR    = '^',
	TOKSTAR   = '*',
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
	TOKHASHHASH,
	TOKDIRSTART,
	TOKDIREND,
	TOKEOF /* EOF needs to be the last. */
};

/* Storage classes */
enum Sclass {
	SCNONE,
	SCEXTERN,
	SCSTATIC,
	SCREGISTER,
	SCGLOBAL,
	SCTYPEDEF,
	SCAUTO
};

struct SrcPos {
	char *file;
	int   line;
	int   col;
};

enum Ctypekind {
	CVOID,
	CPRIM,
	CSTRUCT,
	CARR,
	CPTR,
	CFUNC,
	CENUM,
};

enum PrimKind {
	PRIMCHAR,
	PRIMSHORT,
	PRIMINT,
	PRIMLONG,
	PRIMLLONG,
	PRIMFLOAT,
	PRIMDOUBLE,
	PRIMLDOUBLE
};

struct StructMember {
	int   offset;
	char *name;
	CTy  *type;
};

struct NameTy {
	char *name;
	CTy  *type;
};

struct CTy {
	enum Ctypekind t;
	int size;
	int align;
	int incomplete;
	union {
		struct {
			CTy *rtype;
			Vec *params; /* Vec of *NameTy */
			int  isvararg;
		} Func;
		struct {
			int   isunion;
			int   unspecified;
			char *name;
			Vec  *members;
		} Struct;
		struct {
			CTy *subty;
		} Ptr;
		struct {
			CTy *subty;
			int  dim;
		} Arr;
		struct {
			int issigned;
			int type;
		} Prim;
	};
};

/* node types */
enum Nodekind {
	NASSIGN,
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
	NCOND,
	NINIT,
	NRETURN,
	NSWITCH,
	NGOTO,
	NIDENT,
	NNUM,
	NSTR,
	NIDX,
	NINCDEC,
	NSEL,
	NCALL,
	NSIZEOF,
	NIF,
	NDECL,
	NEXPRSTMT
};

struct Node {
	/* type tag, one of the N* types */
	enum Nodekind t;
	SrcPos pos;
	CTy *type;
	union {
		struct {
			int   localsz;
			char *name;
			Node *body;
			Vec  *params; /* list of *Sym */
			Vec  *locals; /* list of *Sym */
		} Func;
		struct {
			Vec *syms;
		} Decl;
		struct {
			char *lelse;
			Node *expr;
			Node *iftrue;
			Node *iffalse;
		} If;
		struct {
			Node *cond;
			Node *iftrue;
			Node *iffalse;
		} Cond;
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
			char  *l;
			int64 cond;
			Node  *stmt;
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
			int op;
			Node *l;
			Node *r;
		} Assign;
		struct {
			int   op;
			Node *operand;
		} Unop;
		struct {
			int   op;
			int   post;
			Node *operand;
		} Incdec;
		struct {
			Node *operand;
		} Cast;
		struct {
			Vec *inits;
		} Init;
		struct {
			int64 v;
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
			int   arrow;
			char *name;
			Node *operand;
		} Sel;
		struct {
			Node *expr;
		} ExprStmt;
		struct {
			Node *funclike;
			Vec  *args;
		} Call;
		struct {
			CTy *t;
		} Sizeof;
	};
};


struct Sym {
	SrcPos *pos;
	enum Sclass sclass;
	Node   *node;
	CTy    *type;
	char   *name;
	char   *label;  /* SCGLOBAL, SCSTATIC. */
	/* SCAUTO only. */
	union { /* For backend use */
		int   offset;
		char *label; /* For backends which want a text label. */
	} stkloc;
};

#define MAXTOKSZ 4096

struct Lexer {
	FILE   *f;
	SrcPos pos;
	SrcPos prevpos;
	SrcPos markpos;
	int    nchars;
	int    indirective;
	int    ws;
	int    nl;
	char   tokval[MAXTOKSZ+1];
};

typedef struct Tok Tok;
struct Tok {
	enum Tokkind k;
	char   *v;
	int     ws; /* There was whitespace before this token */
	int     nl; /* There was a newline before this token */
	SrcPos  pos;
};

typedef struct Const Const;
struct Const {
	char  *p; /* p is null, or the label of a symbol */
	int64  v;
};

/* dbg.c */
void dumpty(CTy *);

/* error.c */
void errorf(char *, ...);
void errorposf(SrcPos *, char *, ...);

/* lex.c cpp.c */
void  cppinit(char *);
char *tokktostr(int);
Tok  *lex(Lexer *);
Tok  *pp(void);

/* types.c */
int isftype(CTy *);
int isitype(CTy *);
int isarithtype(CTy *);
int isptr(CTy *);
int isfunc(CTy *);
int isfuncptr(CTy *);
int isstruct(CTy *);
int isarray(CTy *);
int sametype(CTy *, CTy *);
int convrank(CTy *);
int canrepresent(CTy *, CTy *);
uint64 getmaxval(CTy *);
int64 getminval(CTy *);
StructMember *getstructmember(CTy *, char *);
CTy *structmemberty(CTy *, char *);
void addstructmember(SrcPos *, CTy *, char *, CTy *);

/* parse.c */
void  parse(void);
char *newlabel();

/* foldexpr.c */
Const *foldexpr(Node *);

/* backend functions */
void  emitinit(FILE *);
void  emitsym(Sym *);


