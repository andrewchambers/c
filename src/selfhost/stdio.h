
typedef struct FILE FILE;
extern FILE *stdout;
int fgetc(FILE *stream);
FILE *fopen(const char *, const char *);
int *fclose(FILE *);
int ungetc(int c, FILE *stream);
int puts(const char *);
int snprintf(char *, long long, const char *, ...);
#define EOF -1
