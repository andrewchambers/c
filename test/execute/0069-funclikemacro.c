#define A 3
#define FOO(X,Y,Z) X + Y + Z
#define SEMI ;

int
main(void)
{
	if(FOO(1, 2, A) != 6)
		return 1 SEMI
	return FOO(0,0,0);
}
