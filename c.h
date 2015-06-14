
#define NDOWHILE 0
#define NFOR 1
#define NNUMBER 2


typedef struct {
	char *file;
	int   line;
	int   col;
} SrcPos;

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
	/* type tag */
	int t;
	union {
		struct {

		} Ptr;
		int prim;
	};
} CTy;

/* TODO: implement enum, then swap */
/* TODO: appropriate start point */
/* TODO: order logically */
#define TOKNUMBER  1000
#define TOKIDENT   1001
#define TOKIF      1002
#define	TOKDO      1003
#define	TOKFOR     1004
#define	TOKWHILE   1005
#define TOKRETURN  1006
#define TOKEOF     1007
#define TOKINC     1008
#define TOKADDASS  1009
#define TOKDEC     1010
#define TOKSUBASS  1011
#define TOKMULASS  1012
#define TOKDIVASS  1013
#define TOKMODASS  1014
#define TOKGTEQL   1015
#define TOKLTEQL   1016
#define TOKNEQL    1017
#define TOKEQL     1018
#define TOKVOID    1019
#define TOKCHAR    1020
#define TOKINT     1021
#define TOKELSE    1022

typedef struct {
	int   k;
	char *v;
	/* Set if the tok was preceeded by an unescaped newline */
	int newline;
	SrcPos pos;
} Tok;

/* 	Singly linked list.
	A null pointer is the empty list. */
typedef struct List {
	struct List *rest;
	void *v;
} List;

void  error(char*, ...);
void  errorpos(SrcPos *, char *, ...);
void *ccmalloc(int);
char *ccstrdup(char*);
/* cpp functions */
char *tokktostr(int);
Tok  *lex(void);
void  cppinit(char *);
/* parser functions */
void  parse(void);
/* backend functions */
void  emit(void);



