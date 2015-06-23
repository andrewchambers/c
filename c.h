
typedef struct ListEnt ListEnt;
struct ListEnt {
	ListEnt *next;
	void *v;
};

typedef struct List List;
struct List {
	ListEnt *head;
};

typedef struct Map Map;
struct Map {
    List *l;
};

/* AST Node types */
enum {
	NDOWHILE,
	NFOR,
	NNUMBER
};

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

typedef struct Node Node;
struct Node {
	/* type tag */
	int t;
	SrcPos pos;
	union {
		struct {

		} If;
		struct {
			Node *stmt;
			Node *expr;
		} DoWhile;
		struct {

		} For;
		struct {

		} While;
		struct {

		} Return;
		struct {

		} Block;
		struct {

		} Decl;
		struct {

		} Func;
		struct {

		} Binop;
		struct {

		} Unop;
		struct {
			/* TODO: parse to int */
			char *v;
		} Number;
	};
};

typedef struct {
	union {
		struct {

		} LSym;
		struct {
			char *name;
		} GSym;
	};
} Sym;

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
};

typedef struct StructMember StructMember;
typedef struct CTy CTy;

struct StructMember {
    char *name;
    CTy  *ty;		    
};

struct CTy {
	/* type tag */
	int t;
	union {
		struct {
			CTy  *rtype;
			List *paramnames;
			List *paramtypes;
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

typedef struct Tok Tok;
struct Tok {
	int   k;
	char *v;
	/* Set if the tok was preceeded by an unescaped newline */
	int newline;
	SrcPos pos;
};

/* helper functions */
void  errorf(char *, ...);
void  errorposf(SrcPos *, char *, ...);
void *ccmalloc(int);
char *ccstrdup(char *);
List *listnew();
void listappend(List *, void *);
void listprepend(List *, void *);
Map  *map();
void *mapget(Map *, char *);
void  mapset(Map *, char *, void *);

/* cpp functions */
char *tokktostr(int);
Tok  *lex(void);
void  cppinit(char *);
/* parser functions */
Node *parse(void);
/* parser functions */
Node *check(Node *);
/* backend functions */
void  emit(Node *);



