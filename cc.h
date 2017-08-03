
typedef struct Node Node;

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

typedef struct Const {
	char  *p; /* p is null, or the label of a symbol */
	int64  v;
} Const;

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
		} Local;
		struct {
			int64 v;
		} Enum;
	};
} Sym;


void compile();

