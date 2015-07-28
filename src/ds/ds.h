/* Data structures */

typedef struct ListEnt ListEnt;
struct ListEnt {
	ListEnt *next;
	void *v;
};

typedef struct List List;
struct List {
	ListEnt *head;
};

List *listnew();
void listappend(List *, void *);
void listprepend(List *, void *);

typedef struct Map Map;
struct Map {
	List *l;
};

Map  *map();
void *mapget(Map *, char *);
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
