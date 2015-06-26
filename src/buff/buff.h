/* Dynamically growing buffer */
typedef struct Buff Buff;
struct Buff {
    char *d;
    int n;
    int sz;
};

Buff *buffnew();
void  bufffree(Buff *);
Buff *buffprintf(Buff *b, char *fmt, ...);

