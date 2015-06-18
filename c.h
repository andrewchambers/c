
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
	TOKGTEQL,
	TOKLTEQL,
	TOKNEQL,
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
	TOKFLOAT,
	TOKDOUBLE,
	TOKSTRUCT,
	TOKUNION,
	TOKGOTO,
	TOKSWITCH,
	TOKREGISTER,
	TOKEXTERN,
	TOKSTATIC,
	TOKAUTO,
	TOKENUM,
	TOKEOF, /* EOF needs to be the last. */
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

} Sym;

typedef struct {
	/* type tag */
	int t;
	int sclass;
	union {
		struct {

		} Ptr;
		int prim;
	};
} CTy;

typedef struct {
	int   k;
	char *v;
	/* Set if the tok was preceeded by an unescaped newline */
	int newline;
	SrcPos pos;
} Tok;

/* 	Singly linked list.
	A null pointer is the empty list. */

typedef struct List List;
struct List {
	List *rest;
	void *v;
};

typedef struct Map Map;
struct Map {
    List *l;
};

/* helper functions */
void  error(char *, ...);
void  errorpos(SrcPos *, char *, ...);
void *ccmalloc(int);
char *ccstrdup(char *);
List *listadd(List *, void *);
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



