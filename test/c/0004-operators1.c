/* TODO: <= >= != == + more types */

int
main()
{
	int x;

	x = 0;
	x = x + 2;        // 2
	x = x - 1;        // 1
	x = x * 6;        // 6
	x = x / 2;        // 3
	x = x % 2;        // 1
	x = x << 2;       // 4
	x = x >> 1;       // 2
	x = x | 255;      // 255
	x = x & 3;        // 3
	x = x ^ 1;        // 2
	x = x + (x > 1);  // 2
	x = x + (x < 3);  // 2
	x = x + (x > 1);  // 3
	x = x + (x < 4);  // 4
    if(x != 4)
    	return 1;
    return 0;
}

