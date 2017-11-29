
int (*fptr)() = 0;


int
main(void)
{
	if (fptr)
		return 1;
	return 0;
}

