
typedef enum {
	SCNONE,
	SCEXTERN,
	SCSTATIC,
	SCREGISTER,
	SCGLOBAL,
	SCTYPEDEF,
	SCAUTO
} Sclass;

typedef enum {
	SYMENUM,
	SYMTYPE,
	SYMGLOBAL,
	SYMLOCAL
} Symkind;

typedef struct Node Node;

typedef struct Sym {
	Symkind k;
	SrcPos *pos;
	CTy    *type;
	char   *name;
	Node   *init;
	union {
		struct {
			int sclass;
			char *label;
		} Global;
		struct {
			int paramidx;
			int isparam;
			IRVal addr;
		} Local;
		struct {
			int64 v;
		} Enum;
	};
} Sym;

typedef struct Const {
	char  *p;
	int64  v;
} Const;

typedef enum {
	NASSIGN,
	NBINOP,
	NUNOP,
	NCALL,
	NCAST,
	NCOMMA,
	NCOND,
	NINIT,
	NIDENT,
	NNUM,
	NSTR,
	NIDX,
	NINCDEC,
	NPTRADD,
	NSEL,
	NSIZEOF,
	NBUILTIN
} Nodekind;

typedef struct {
	int offset;
	Node *n;
} InitMember;

typedef enum {
	BUILTIN_VASTART
} Builtinkind;

struct Node {
	/* type tag, one of the N* types */
	Nodekind t;
	SrcPos pos;
	CTy *type;
	union {
		struct {
			Vec *exprs;
		} Comma;
		struct {
			int op;
			Node *l;
			Node *r;
		} Binop;
		struct {
			Node *cond;
			Node *iftrue;
			Node *iffalse;
		} Cond;
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
			Node *ptr;
			Node *offset;
		} Ptradd;
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
			Node *funclike;
			Vec  *args;
		} Call;
		struct {
			CTy *type;
		} Sizeof;
		struct {
			Builtinkind t;
			union {
				struct {
					Node *valist;
					Node *param;
				} Vastart;
			};
		} Builtin;
	};
};

typedef struct Case {
	SrcPos *pos;
	char *label;
	int64 v;
} Case;

typedef struct Switch {
	SrcPos *pos;
	Vec  *cases;
	char *defaultlabel;
} Switch;

typedef struct Data {
	char *label;
	CTy  *type;
	Node *init;
	int   isglobal;
} Data;

typedef struct GotoLabel {
	int defined;
	char *backendlabel;
} GotoLabel;

typedef struct Goto {
	SrcPos pos;
	GotoLabel *label;
} Goto;

void setoutput(FILE *);
void compile();
