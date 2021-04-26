typedef struct Pic Pic;

extern char *prefix;

enum{
	Vwidth = 320,
	Vheight = 210,
};

extern int npal;
extern u32int *pal;

enum{
	PCfont,
	PCarrow,
	PCspace,
	PCgrid,
	PCplanets,
	PCship,
	PChud,
	PCface,
	PCammo,
	PChit,
	PCdir,
	PCcur,
	PCscroll,
	PCgibs,
	PCend,
};
struct Pic{
	u32int *p;
	int w;
	int h;
};
extern Pic pics[PCend];
