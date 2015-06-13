
extern char  tokval[];
extern int   tok;
extern char *fname;
extern int   line;
extern int   col;

void  error(char*, ...);
void *ccmalloc(int);
void  next(void);
void  cppinit(char *);
void  parse(void);
void  emit(void);
char *tok2str(int);

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
/* TODO: appropriate start point */

#define TOKNUMBER 1000
#define TOKIDENT  1001
#define TOKIF     1002
#define	TOKDO     1003
#define	TOKFOR    1004
#define	TOKWHILE  1005
#define TOKRETURN 1006
#define TOKEOF    1007
#define TOKPOSINC 1008
#define TOKADDASS 1009
#define TOKPOSDEC 1010
#define TOKSUBASS 1011
#define TOKMULASS 1012
#define TOKDIVASS 1013
#define TOKMODASS 1014
#define TOKGTEQL  1015
#define TOKLTEQL  1016
#define TOKNEQL   1017
#define TOKEQL    1018