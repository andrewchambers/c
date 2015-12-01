
typedef struct {
    unsigned int gp_offset;
    unsigned int fp_offset;
    void *overflow_arg_area;
    void *reg_save_area;
} __va_elem;

typedef __va_elem va_list[1];

#define va_start(X, Y) __builtin_va_start((X), (Y))
#define va_end(X) (X)

int vfprintf(FILE *stream, const char *format, va_list ap);

