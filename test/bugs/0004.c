
int
main()
{
	int i, n, p, next, isprime;
	
	n = 5;
	p = 11;
	next = 12;
	while(n != 100) {
		isprime = 1;
		if(next % 2 == 0) {
			isprime = 0;
		} else {
			for(i = 3; i < next; i = i + 2) {
				if(next % i == 0) {
					isprime = 0;
					break;
				}
			}
		}
		if(isprime) {
			p = next;
			n++;
		}
		next = next + 1;
	}
	if(p != 541)
		return 1;
	return 0;
}
