int
main()
{
	int x;
	if((sizeof (int) - 4))
		return 1;
	if((sizeof (&x) - 8))
		return 1;
	return 0;
}
