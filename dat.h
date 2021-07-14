typedef struct Wall Wall;
typedef struct Pic Pic;

extern char *prefix;

enum{
	Vw = 320,
	Vfullh = 210,
	Vdrwh = Vfullh - 60 - 2,	/* FIXME: ? */
	Vh = Vdrwh + 40,
	Vcenterx = Vw / 2,
	Vcentery = Vh / 2,
	Vfntw = 7,
	Vfnth = 12,
	Vfntspc = 1,
	Vfntpicw = Vfntspc+Vfntw+Vfntspc,

	Wallsz = 64,
	Wtexelsz = Wallsz / 2,

	Kfire = 0,
	K↑,
	K↓,
	K←,
	K→,
};

enum{
	BScontinue = 114,
	BSgameloaded,
	BSmore,
	BSintro1 = 256,
	BSintro2,
	BSintro3,
	BSend = 270,
};
extern char **basestr;

extern int npal;
extern u32int *pal;

struct Wall{
	u32int p[Wallsz*Wallsz];
};
extern Wall *walls;
extern int nwalls;

enum{
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
extern Pic pics[PCend], canvas;
extern nfontmap;
extern uchar *fontmap;
extern int nglyph;
extern Pic *dfont;

enum{
	DShip = 0xffbb0000,
	DHud1 = 0xff313131,
	DHud2 = 0xff808591,
	DFace = 0xff323232,
	DAmmo = 0xff828282,
};

extern void (*step)(void);
extern void (*input)(Rune);
extern vlong tc;
