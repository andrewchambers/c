
#define EOF -1

typedef struct FILE FILE;
extern FILE *stdout, *stderr;


int   ungetc(int c, FILE *stream);
int   puts(const char *);
int   snprintf(char *, long long, const char *, ...);
int   fprintf(FILE *stream, const char *format, ...);
int   fgetc(FILE *stream);
int   fputs(const char *s, FILE *stream);
int   fputc(int c, FILE *stream);
FILE *fopen(const char *, const char *);
int  *fclose(FILE *);

