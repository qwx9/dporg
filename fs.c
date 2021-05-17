#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include "dat.h"
#include "fns.h"

s32int sintab[256];
char **basestr;
int nfontmap;
uchar *fontmap;
int nglyph;
Pic *dfont;

static Biobuf *
eopen(char *s, int mode)
{
	Biobuf *bf;

	if((bf = Bopen(s, mode)) == nil)
		sysfatal("Bopen: %r");
	Blethal(bf, nil);
	return bf;
}

static long
eread(Biobuf *bf, void *buf, long n)
{
	if(Bread(bf, buf, n) != n)
		sysfatal("ebread: short read: %r");
	return n;
}

static vlong
bsize(Biobuf *bf)
{
	vlong n;
	Dir *d;

	d = dirfstat(Bfildes(bf));
	if(d == nil)
		sysfatal("bstat: %r");
	n = d->length;
	free(d);
	return n;
}

static u8int
get8(Biobuf *bf)
{
	u8int v;

	eread(bf, &v, 1);
	return v;
}

static u16int
get16(Biobuf *bf)
{
	u16int v;

	v = get8(bf);
	return v | get8(bf) << 8;
}

static u32int
get24(Biobuf *bf)
{
	u32int v;

	v = get16(bf);
	return v | get8(bf) << 16;
}

static u32int
get32(Biobuf *bf)
{
	u32int v;

	v = get16(bf);
	return v | get16(bf) << 16;
}

static void
loadpic(char *name, Pic *pic)
{
	int fd, n, m, dx, dy;
	uchar *b, *s;
	u32int *p;
	Image *i;

	if((fd = open(name, OREAD)) < 0)
		sysfatal("loadpic: %r");
	if((i = readimage(display, fd, 0)) == nil)
		sysfatal("readimage: %r");
	close(fd);
	if(i->chan != RGBA32)
		sysfatal("loadpic %s: inappropriate image format", name);
	dx = Dx(i->r);
	dy = Dy(i->r);
	n = dx * dy;
	if(dx * dy > Vw * Vfullh)
		sysfatal("loadpic %s: inappropriate image size", name);
	p = emalloc(n * sizeof *p);
	pic->p = p;
	pic->w = dx;
	pic->h = dy;
	m = i->depth / 8;
	b = emalloc(n * m);
	unloadimage(i, i->r, b, n * m);
	freeimage(i);
	s = b;
	while(n-- > 0){
		*p++ = s[0] << 24 | s[3] << 16 | s[2] << 8 | s[1];
		s += m;
	}
	free(b);
}

static void
loadfont(void)
{
	int i, n, nx, ny;
	u32int *d, *s;
	Pic pic, *pp;

	loadpic("a.bit", &pic);
	nx = pic.w / Vfntpicw;
	ny = pic.h / Vfnth;
	if(pic.w % nx != 0 || pic.h % ny != 0)
		sysfatal("loadfont: invalid font pic");
	nglyph = nx * ny;
	dfont = emalloc(nglyph * sizeof *dfont);
	for(i=0, pp=dfont; pp<dfont+nglyph; pp++, i++){
		pp->w = Vfntw;
		pp->h = Vfnth;
		pp->p = emalloc(pp->w * pp->h * sizeof *pp->p);
		d = pp->p;
		s = pic.p + i / nx * pic.w * pp->h + i % nx * Vfntpicw + Vfntspc;
		for(n=0; n<pp->h; n++){
			memcpy(d, s, pp->w * sizeof *d);
			d += pp->w;
			s += pic.w;
		}
	}
	free(pic.p);
}

void
loadpics(void)
{
	loadpic("b.bit", pics + PCarrow);
	loadpic("c.bit", pics + PCspace);
	loadpic("d.bit", pics + PCgrid);
	loadpic("e.bit", pics + PCplanets);
	loadpic("f.bit", pics + PCship);
	loadpic("k.bit", pics + PChud);
	loadpic("l.bit", pics + PCface);
	loadpic("m.bit", pics + PCammo);
	loadpic("n.bit", pics + PChit);
	loadpic("o.bit", pics + PCdir);
	loadpic("p.bit", pics + PCcur);
	loadpic("q.bit", pics + PCscroll);
	loadpic("r.bit", pics + PCgibs);
	loadfont();
	canvas.p = emalloc(Vw * Vfullh * sizeof *canvas.p);
	canvas.w = Vw;
	canvas.h = Vfullh;
}

static void
loadpal(void)
{
	int n;
	u8int r, g, b;
	u32int *p;
	Biobuf *bf;

	bf = eopen("palettes.bin", OREAD);
	npal = get32(bf) / 2;
	pal = emalloc(npal * sizeof *pal);
	for(p=pal; p<pal+npal; p++){
		n = get16(bf);
		r = n & 0x1f;
		r = r << 3 | r >> 2;
		g = n >> 5 & 0x3f;
		g = g << 2 | g >> 4;
		b = n >> 11 & 0x1f;
		b = b << 3 | b >> 2;
		*p = 0xff << 24 | r << 16 | g << 8 | b;
	}
	Bterm(bf);
}

static char **
loadstr(char *name, int *nel)
{
	int n;
	char **s, **p, **e, *q;
	Biobuf *bf;

	bf = eopen(name, OREAD);
	n = get16(bf);
	s = emalloc(n * sizeof *p);
	e = s + n;
	*nel = n;
	for(p=s; p<e; p++){
		n = get16(bf);
		*p = emalloc(n + 1);
		eread(bf, *p, n);
		/* FIXME: not always? */
		q = *p;
		while((q = strchr(q, '|')) != nil)
			*q = '\n';
	}
	Bterm(bf);
	return s;
}

static void
loadfontmap(void)
{
	int n;
	Biobuf *bf;

	bf = eopen("a.map", OREAD);
	n = bsize(bf);
	fontmap = emalloc(n * sizeof *fontmap);
	eread(bf, fontmap, n);
	nfontmap = n;
	Bterm(bf);
}

static void
loadbasestr(void)
{
	int nel;

	basestr = loadstr("base.str", &nel);
	if(nel != BSend)
		sysfatal("loadbasestr: inconsistent base.str file: entries %d not %d", nel, BSend);
}

static void
loadsintab(void)
{
	Biobuf *bf;
	s32int *s;

	bf = eopen("sintable.bin", OREAD);
	for(s=sintab; s<sintab+nelem(sintab); s++)
		*s = get32(bf);
	Bterm(bf);
}

void
initfs(void)
{
	rfork(RFNAMEG);
	if(bind(".", prefix, MBEFORE|MCREATE) == -1 || chdir(prefix) < 0)
		fprint(2, "initfs: %r\n");
	loadsintab();
	loadpal();
	loadbasestr();
	loadfontmap();
}

// grids: 32/256/2048
// wtexels.bin
// stexels.bin
// bitshapes.bin
// entities.db
// entities.str
// mappings.bin
// map bsp + str (on demand, with shim load gauge)
