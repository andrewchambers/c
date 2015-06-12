
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
#define TOKIF     1001
#define	TOKDO     1002
#define	TOKFOR    1003
#define	TOKWHILE  1004
#define TOKRETURN 1005
#define TOKEOF    1006
