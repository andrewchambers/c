#define A 3
#define FOO(X,Y,Z) X + Y + Z
#define SEMI ;
#define Marva 10;
int
main()
{
	if(FOO(1, 2, A) != 6)
		printf("Marva %d",Marva);
		return 1 SEMI
	return FOO(0,0,0);
}
