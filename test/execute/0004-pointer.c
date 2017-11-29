

int
main(void)
{
	int x;
	int *p;
	
	x = 4;
	p = &x;
	*p = 0;

	return *p;
}
