struct T {
	int x;
	int y;
};

int
main()
{
	struct T v;
	
	v.x = 2;
	v.y = 5;
	if(v.x + v.y != 7)
		return 1;
	return 0;
}
