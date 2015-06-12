
extern char *tokval;
extern int   tok;
extern char *fname;
extern int   line;
extern int   col;

void error(char*);
void *ccmalloc(int);
int  next();
void cppinit();
void parse();
void emit();

typedef struct {
	int t;
	union {
		struct {

		} If;
		struct {

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
	};
} Node;

/* TODO: implement enum, then swap. */
#define TOKIF     1001 /* TODO: appropriate start point */
#define	TOKDO     1002
#define	TOKFOR    1003
#define	TOKWHILE  1004
#define TOKRETURN 1005
#define TOKEOF    1006
#define TOKPOSINC 1007
#define TOKADDASS 1008
#define TOKPOSDEC 1009
#define TOKSUBASS 1010
#define TOKMULASS 1011
#define TOKDIVASS 1012
#define TOKMODASS 1013
#define TOKGTEQL  1014
#define TOKLTEQL  1015
#define TOKNEQL   1016
#define TOKEQL    1017