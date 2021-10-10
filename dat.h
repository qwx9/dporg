typedef struct Wall Wall;
typedef struct Pic Pic;
typedef struct Sprite Sprite;
typedef struct Map Map;
typedef struct Node Node;
typedef struct Line Line;

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

	Npalcol = 16,
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

struct Sprite{
	Pic;
	Rectangle r;
};
extern Sprite *sprites;
extern int nsprites;

enum{
	DShip = 0xffbb0000,
	DHud1 = 0xff313131,
	DHud2 = 0xff808591,
	DFace = 0xff323232,
	DAmmo = 0xff828282,
};

struct Node{
	int type;
	Rectangle;
	int split;
	int left;
	int right;
};
enum{
	LFwall = 0<<0,
	LFdoor = 1<<0 | 1<<2,
	LFshiftS⋁E = 1<<3,
	LFshiftN∨W = 1<<4,
	LFsecret = 1<<5,
	LFyshift = 1<<8,
	LFxshift = 1<<9,
	LFlock = 1<<10,
	LFwallS = 1<<11,
	LFwallN = 1<<12,
	LFwallW = 1<<13,
	LFwallE = 1<<14,
	LFmirrored = 1<<15,

	Fineshift = 3,
};
struct Line{
	int flags;
	int tex;
	int unkn1;
	Rectangle;
	int minpshift;
	int maxpshift;
};
struct Map{
	int id;
	int nnodes;
	Node *nodes;
	int nlines;
	Line *lines;
	u32int ceilc;
	u32int floorc;
	u32int backc;
	int unkn1;
	int unkn2;
};
extern Map map;

extern void (*step)(void);
extern void (*input)(Rune);
extern vlong tc;
