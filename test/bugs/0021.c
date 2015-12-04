char *x = "abc";

int
main()
{
    char *p;

    p = x;
    p = p + 1;
    if(p[0] != 'b')
        return 1;
    return 0;
}
