
typedef struct FILE FILE;
extern FILE *stdout;
FILE *fopen(const char *, const char *);
int *fclose(FILE *);
int puts(const char *);
int snprintf(char *, long long, const char *, ...);
