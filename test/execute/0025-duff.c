int main()
{
	int  count, n;
	char *from, *to;
	char a[39], b[39];

	from = a;
	to = b;
	from[0] = 1;
	from[1] = 2;
	from[2] = 3;
	count = 39;
	n = (count + 7) / 8;
	switch (count % 8) {
	case 0: do { *to++ = *from++;
	case 7:      *to++ = *from++;
	case 6:      *to++ = *from++;
	case 5:      *to++ = *from++;
	case 4:      *to++ = *from++;
	case 3:      *to++ = *from++;
	case 2:      *to++ = *from++;
	case 1:      *to++ = *from++;
			} while (--n > 0);
	}
	if(a[0] != 1)
		return 1;
	if(a[1] != 2)
		return 1;
	if(a[2] != 3)
		return 1;
	return 0;
}

