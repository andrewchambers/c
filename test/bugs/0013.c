/* seed 3533508740 */
struct s0 {
	long m0;
	long m1;
};
struct s1 {
	struct s0 m0;
	long m1;
};
struct s2 {
	int m0;
};
void abort(void);
struct s1
f(int p0, struct s2 p1)
{
	struct s1 r;
	r.m0.m0 = 1293431308;
	r.m0.m1 = 61651767;
	r.m1 = 1736112203;
	if(p0 != 413324820) abort();
	if(p1.m0 != 1141848071) abort();
	return r;
}
int
main()
{
	struct s1 r;
	int p0;
	struct s2 p1;
	p0 = 413324820;
	p1.m0 = 1141848071;
	r = f(p0, p1);
	if(r.m0.m0 != 1293431308) abort();
	if(r.m0.m1 != 61651767) abort();
	if(r.m1 != 1736112203) abort();
	return 0;
}
