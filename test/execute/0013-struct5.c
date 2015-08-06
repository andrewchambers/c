struct s1 {
    int y;
    int z;
};

struct s2 {
    struct s1 *p;
};

int main()
{
    struct s1 nested;
    struct s2 v;
    v.p = &nested;
    v.p->y = 1;
    v.p->z = 2;
    if (nested.y != 1)
        return 1;
    if (nested.z != 2)
        return 2;
    return 0;
}
