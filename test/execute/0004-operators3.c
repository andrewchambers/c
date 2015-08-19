int
main()
{
	int x;

	x = 0;
	x += 1;
	if (x != 1)
		return 1;
	x -= 1;
	if(x != 0)
		return 1;
	return 0;
}

