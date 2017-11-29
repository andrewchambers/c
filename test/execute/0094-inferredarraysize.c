

int a[] = {1, 2, 3, 4};

int
main(void)
{
	if (sizeof(a) != 4*sizeof(int))
		return 1;
	
	return 0;
}
