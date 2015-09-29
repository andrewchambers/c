/* Provides generic data structures. */

typedef struct ListEnt ListEnt;
struct ListEnt {
	ListEnt *next;
	void *v;
};

typedef struct List List;
struct List {
	int     len;
	ListEnt *head;
};

List *list();
void listappend(List *, void *);
void listprepend(List *, void *);
void listinsert(List *, int, void *);
void *listpopfront(List *);

typedef struct Map Map;
struct Map {
	List *l;
};

Map  *map();
void *mapget(Map *, char *);
void  mapdel(Map *, char *);
void  mapset(Map *, char *, void *);

/*	StrSet is an immutable set of strings.
	The null pointer is the empty set. */
typedef struct StrSet StrSet;
struct StrSet {
	StrSet *next;
	char *v;
};

StrSet *strsetadd(StrSet *, char *);
int     strsethas(StrSet *, char *);
StrSet *strsetintersect(StrSet *, StrSet *);

typedef struct Vec Vec;
struct Vec {
	int   cap;
	int   len;
	void  **d;
};

Vec  *vec();
void *vecget(Vec *, int);
void  vecset(Vec *, int, void *);
void  vecappend(Vec *, void *);
