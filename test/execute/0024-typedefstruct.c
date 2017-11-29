
typedef struct { int x; int y; } s;

s v;

int
main(void)
{
	v.x = 1;
	v.y = 2;
	return 3 - v.x - v.y;
}

