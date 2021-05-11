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
extern Pic pics[PCend], canvas;

enum{
	DShip = 0xbb0000ff,
	DHud1 = 0x313131ff,
	DHud2 = 0x808591ff,
	DFace = 0x323232ff,
	DAmmo = 0x828282ff,
};

extern void (*step)(void);
extern void (*input)(Rune);
extern vlong tc;
