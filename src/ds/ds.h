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

typedef struct Map Map;
struct Map {
    List *l;
};

List *listnew();
void listappend(List *, void *);
void listprepend(List *, void *);

Map  *map();
void *mapget(Map *, char *);
void  mapset(Map *, char *, void *);

