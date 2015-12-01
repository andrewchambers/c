
typedef struct {
    unsigned int gp_offset;
    unsigned int fp_offset;
    void *overflow_arg_area;
    void *reg_save_area;
} __va_elem;

typedef __va_elem va_list[1];

#define va_start(X, Y) do { \
                            (X)[0].gp_offset = 0; \
                            (X)[0].fp_offset = 0; \
                            (X)[0].overflow_arg_area = 0; \
                            (X)[0].reg_save_area = 0;\
                       } while(0);
#define va_end(X) (X)

int vfprintf(FILE *stream, const char *format, va_list ap);

