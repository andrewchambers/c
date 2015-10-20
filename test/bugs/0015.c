

int printf(char *, ...);
int *calloc(int, int);

int N;
int *t;

void
print()
{
        int x;
        int y;

        for (y=0; y<8; y++) {
                for (x=0; x<8; x++)
                        if (t[x + 8*y])
                                printf(" Q");
                        else
                                printf(" .");
                printf("\n");
        }
        printf("\n");
}

int
chk(int x, int y)
{
        int i;
        int r;

        for (r=i=0; i<8; i++) {
                r = r + t[x + 8*i];
                r = r + t[i + 8*y];
                if (x+i < 8 & y+i < 8)
                        r = r + t[x+i + 8*(y+i)];
                if (x+i < 8 & y-i >= 0)
                        r = r + t[x+i + 8*(y-i)];
                if (x-i >= 0 & y+i < 8)
                        r = r + t[x-i + 8*(y+i)];
                if (x-i >= 0 & y-i >= 0)
                        r = r + t[x-i + 8*(y-i)];
        }
        return r;
}

int
go(int n, int x, int y)
{
        if (n == 8) {
                print();
                N++;
                return 0;
        }
        for (; y<8; y++) {
                for (; x<8; x++)
                        if (chk(x, y) == 0) {
                                t[x + 8*y]++;
                                go(n+1, x, y);
                                t[x + 8*y]--;
                        }
                x = 0;
        }
}

int
main()
{
        t = calloc(64, sizeof(int));
        go(0, 0, 0);
        printf("found %d solutions\n", N);
        if(N != 92)
        	return 1;
        return 0;
}

