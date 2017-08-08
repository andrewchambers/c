typedef enum {
        CVOID,
        CPRIM,
        CSTRUCT,
        CARR,
        CPTR,
        CFUNC,
        CENUM
} Ctypekind;

typedef enum {
        PRIMCHAR,
        PRIMSHORT,
        PRIMINT,
        PRIMLONG,
        PRIMLLONG,
        PRIMFLOAT,
        PRIMDOUBLE,
        PRIMLDOUBLE
} Primkind;

typedef struct StructMember StructMember;
typedef struct StructExport StructExport;
typedef struct StructIter StructIter;
typedef struct ExportPath ExportPath;
typedef struct NameTy NameTy;
typedef struct CTy CTy;

struct StructMember {
        int   offset;
        char *name;
        CTy  *type;
};

struct NameTy {
        char *name;
        CTy  *type;
};

struct StructIter {
        CTy *root;
        int depth;
        int path[1024];
};

struct ExportPath {
        int idx;
        ExportPath *next;
};

struct StructExport {
        char *name;
        ExportPath *path;
};

struct CTy {
        Ctypekind t;
        int size;
        int align;
        int incomplete;
        union {
                struct {
                        CTy *rtype;
                        Vec *params; /* Vec of *NameTy */
                        int  isvararg;
                } Func;
                struct {
                        int   isunion;
                        char *name;
                        Vec  *members;
                        Vec  *exports;
                } Struct;
                struct {
                        Vec  *members;
                } Enum;
                struct {
                        CTy *subty;
                } Ptr;
                struct {
                        CTy *subty;
                        int  dim;
                } Arr;
                struct {
                        int issigned;
                        int type;
                } Prim;
        };
};


CTy *newtype(int);
CTy *mkptr(CTy *);
int isvoid(CTy *);
int isftype(CTy *);
int isitype(CTy *);
int isarithtype(CTy *);
int isptr(CTy *);
int ischarptr(CTy *t);
int ischararray(CTy *t);
int isfunc(CTy *);
int isfuncptr(CTy *);
int isstruct(CTy *);
int isarray(CTy *);
int issignedty(CTy *)
int sametype(CTy *, CTy *);
int convrank(CTy *);
int canrepresent(CTy *, CTy *);
int getstructiter(StructIter *, CTy *, char *);
int structnext(StructIter *);
int structoffsetfromname(CTy *, char *);
CTy * structtypefromname(CTy *, char *);
void initstructiter(StructIter *, CTy *);
void structwalk(StructIter *, StructMember **, int *);
void addtostruct(CTy *, char *, CTy *);
void finalizestruct(SrcPos *, CTy *);
NameTy *newnamety(char *n, CTy *t);


uint64 getmaxval(CTy *);
int64  getminval(CTy *);

extern CTy *cvoid;
extern CTy *cchar;
extern CTy *cshort;
extern CTy *cint;
extern CTy *clong;
extern CTy *cllong;
extern CTy *cuchar;
extern CTy *cushort;
extern CTy *cuint;
extern CTy *culong;
extern CTy *cullong;
extern CTy *cfloat;
extern CTy *cdouble;
extern CTy *cldouble;


