
int
main()
{
	int x;
	
	x = 0;
	x += 2;
	x += 2;
	if (x != 4)
		return 1;
	x -= 3;
	if (x != 1)
		return 2;
		
	return 0;
}
