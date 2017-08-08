
int strcmp(const char *s1, const char *s2);
int snprintf(char *str, int size, const char *format, ...);

int
main()
{
	char buf[128];
	
	snprintf(buf, sizeof(buf), "hello %s", "world!");
	
	if (strcmp("hello world!", buf) != 0)
		return 1;

	return 0;
}
