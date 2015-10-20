struct s {
	char a;
	char b;
};

void abort(void);

struct s
f()
{
	struct s r;
	r.a = 1;
	r.b = 2;
	return r;
}
int
main()
{
	struct s r;
	r = f();
	if(r.a != 1) abort();
	if(r.b != 2) abort();
	return 0;
}
